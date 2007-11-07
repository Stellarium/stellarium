/*
 * Stellarium
 * This file Copyright (C) 2005-2006 Robert Spearman
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
#include "StelCore.hpp"
#include "StelApp.hpp"
#include "audio.h"
#include "image.h"
#include "stel_ui.h"
#include "script_mgr.h"
#include "StarMgr.hpp"
#include "ConstellationMgr.hpp"
#include "SolarSystem.hpp"
#include "NebulaMgr.hpp"
#include "LandscapeMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "MovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "image_mgr.h"
#include "MeteorMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "Projector.hpp"
#include "Navigator.hpp"
#include "StelFileMgr.hpp"
#include "Planet.hpp"
#include "Observer.hpp"

using namespace std;


StelCommandInterface::StelCommandInterface(StelCore * core, StelApp * app) : FlagTimePause(0)
{
	setObjectName("StelCommandInterface");
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
	map<string, string> args;
	int status = 0;  // true if command was understood
	int recordable = 1;  // true if command should be recorded (if recording)
	wstring debug_message;  // detailed errors can be placed in here for printout at end of method

	wait = 0;  // default, no wait between commands

	status = parse_command(commandline, command, args);

	ConstellationMgr* cmgr = (ConstellationMgr*)StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr");
	StarMgr* smgr = (StarMgr*)StelApp::getInstance().getModuleMgr().getModule("StarMgr");
	NebulaMgr* nmgr = (NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr");
	SolarSystem* ssmgr = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	LandscapeMgr* lmgr = (LandscapeMgr*)StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr");
	MovementMgr* mvmgr = (MovementMgr*)StelApp::getInstance().getModuleMgr().getModule("MovementMgr");
	MeteorMgr* metmgr = (MeteorMgr*)StelApp::getInstance().getModuleMgr().getModule("MeteorMgr");
	ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("ScriptMgr");
	
	// stellarium specific logic to run each command

	if(command == "flag") {

		// could loop if want to allow that syntax
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
		float fdelay = StelUtils::stringToDouble(args["duration"]);
		if(fdelay > 0) wait = (int)(fdelay*1000);

	} else if (command == "set") {
		// set core variables

		// TODO: some bounds/error checking here
		
		if(args["atmosphere_fade_duration"]!="") lmgr->setAtmosphereFadeDuration(StelUtils::stringToDouble(args["atmosphere_fade_duration"]));
		else if(args["auto_move_duration"]!="") mvmgr->setAutomoveDuration( StelUtils::stringToDouble(args["auto_move_duration"]));
		else if(args["constellation_art_fade_duration"]!="") cmgr->setArtFadeDuration(StelUtils::stringToDouble(args["constellation_art_fade_duration"]));
		else if(args["constellation_art_intensity"]!="") cmgr->setArtIntensity(StelUtils::stringToDouble(args["constellation_art_intensity"]));
		else if(args["home_planet"]!="") stcore->setHomePlanet(args["home_planet"]);
		else if(args["landscape_name"]!="") lmgr->setLandscape(args["landscape_name"]);
		else if(args["light_pollution_luminance"]!="") 
			lmgr->setAtmosphereLightPollutionLuminance(StelUtils::stringToDouble(args["light_pollution_luminance"]));
		else if(args["max_mag_nebula_name"]!="") nmgr->setMaxMagHints(StelUtils::stringToDouble(args["max_mag_nebula_name"]));
		else if(args["max_mag_star_name"]!="") smgr->setMaxMagName(StelUtils::stringToDouble(args["max_mag_star_name"]));
		else if(args["moon_scale"]!="") ssmgr->setMoonScale(StelUtils::stringToDouble(args["moon_scale"]));
		else if(args["sky_culture"]!="") stapp->getSkyCultureMgr().setSkyCultureDir(args["sky_culture"]);
		else if(args["sky_locale"]!="") stapp->getLocaleMgr().setSkyLanguage(args["sky_locale"]);
		else if(args["star_mag_scale"]!="") smgr->setMagScale(StelUtils::stringToDouble(args["star_mag_scale"]));
		else if(args["star_scale"]!="") {
			float scale = StelUtils::stringToDouble(args["star_scale"]);
			smgr->setScale(scale);
			ssmgr->setScale(scale);
		} 
		else if(args["nebula_scale"]!="")
			{
				float scale = StelUtils::stringToDouble(args["nebula_scale"]);
				nmgr->setCircleScale(scale);
			} 
		else if(args["star_twinkle_amount"]!="") smgr->setTwinkleAmount(StelUtils::stringToDouble(args["star_twinkle_amount"]));
		else if(args["time_zone"]!="") stapp->getLocaleMgr().setCustomTimezone(args["time_zone"]);
		else if(args["milky_way_intensity"]!="") {
			MilkyWay* mw = (MilkyWay*)stapp->getModuleMgr().getModule("MilkyWay");
			mw->setIntensity(StelUtils::stringToDouble(args["milky_way_intensity"]));
			// safety feature to be able to turn back on
			if(mw->getIntensity()) mw->setFlagShow(true);

		} else status = 0;


		if(trusted) {

			// trusted commands disabled due to code reorg

			//    else if(args["base_font_size"]!="") stcore->BaseFontSize = StelUtils::stringToDouble(args["base_font_size"]);
			//	else if(args["bbp_mode"]!="") stcore->BbpMode = StelUtils::stringToDouble(args["bbp_mode"]);
			//    else if(args["date_display_format"]!="") stcore->DateDisplayFormat = args["date_display_format"];
			//	else if(args["fullscreen"]!="") stcore->Fullscreen = args["fullscreen"];
			//	else if(args["horizontal_offset"]!="") stcore->HorizontalOffset = StelUtils::stringToDouble(args["horizontal_offset"]);
			//	else if(args["init_fov"]!="") stcore->InitFov = StelUtils::stringToDouble(args["init_fov"]);
			//	else if(args["preset_sky_time"]!="") stapp->PresetSkyTime = StelUtils::stringToDouble(args["preset_sky_time"]);
			//	else if(args["screen_h"]!="") stcore->ScreenH = StelUtils::stringToDouble(args["screen_h"]);
			//	else if(args["screen_w"]!="") stcore->ScreenW = StelUtils::stringToDouble(args["screen_w"]);
			//    else if(args["startup_time_mode"]!="") stapp->StartupTimeMode = args["startup_time_mode"];
			// else if(args["time_display_format"]!="") stcore->TimeDisplayFormat = args["time_display_format"];
			//else if(args["type"]!="") stcore->Type = args["type"];
			//else if(args["version"]!="") stcore->Version = StelUtils::stringToDouble(args["version"]);
			//      else if(args["vertical_offset"]!="") stcore->VerticalOffset = StelUtils::stringToDouble(args["vertical_offset"]);
			//else if(args["viewing_mode"]!="") stcore->ViewingMode = args["viewing_mode"];
			//else if(args["viewport"]!="") stcore->Viewport = args["viewport"];

		}

	} else if (command == "select") {

		// default is to unselect current object (TODO: but not constellations!)

		
		if(args["hp"]!=""){
			StelApp::getInstance().getStelObjectMgr().setSelectedObject( smgr->searchByName(string("HP") + args["hp"]), false);
		}
		else if(args["star"]!="") {
			StelApp::getInstance().getStelObjectMgr().setSelectedObject( smgr->searchByName(args["star"]), false);
		} 
		else if(args["planet"]!=""){
			StelApp::getInstance().getStelObjectMgr().setSelectedObject( ssmgr->searchByName(args["planet"]), false);
		} 
		else if(args["nebula"]!=""){
			StelApp::getInstance().getStelObjectMgr().setSelectedObject( nmgr->searchByName(args["nebula"]), false);
		} 
		else if(args["constellation"]!=""){
			StelApp::getInstance().getStelObjectMgr().setSelectedObject( cmgr->searchByName(args["constellation"]), true);
		} 
		else if(args["constellation_star"]!=""){
			// This is no longer supported as easy to just select a star.  Here for backward compatibility.
			StelApp::getInstance().getStelObjectMgr().setSelectedObject( cmgr->searchByName(args["constellation_star"]), true);
			cout << "WARNING: select constellation_star comand is no longer fully supported.\n";
		} else {
			// unselect current selection 
			// TODO: should not deselect constellations?
			StelApp::getInstance().getStelObjectMgr().unSelect();
		}


		//		if(select_type != "" ) StelApp::getInstance().getStelObjectMgr().findAndSelect(identifier, addToSelection);
		
		// determine if selected object pointer should be displayed
		// TODO also make this a set option
		if(args["pointer"]=="off" || args["pointer"]=="0") StelApp::getInstance().getStelObjectMgr().setFlagSelectedObjectPointer(false);
		else StelApp::getInstance().getStelObjectMgr().setFlagSelectedObjectPointer(true);
    

	} else if (command == "deselect") {
		/* TODO
		if(args["constellation"] != "") { 
			stcore->unsetSelectedConstellation(args["constellation"]);
		} else {
			stcore->deselect();
		}
		*/

		StelApp::getInstance().getStelObjectMgr().unSelect();

	} else if(command == "look") {  // change direction of view

		if(args["delta_az"]!="" || args["delta_alt"]!="") {
			// immediately change viewing direction
			double dAz = 0.0;
			double dAlt = 0.0;
			if (args["delta_az"]!="") dAz = StelUtils::stringToDouble(args["delta_az"]);
			if (args["delta_alt"]!="") dAlt = StelUtils::stringToDouble(args["delta_alt"]);
			mvmgr->panView(dAz, dAlt);
		}	else status = 0;

		// TODO absolute settings (see RFE 1311031)


	  

	} else if(command == "zoom") {
		double duration=0.0;
		if (args["duration"]!="") duration = StelUtils::stringToDouble(args["duration"]);

		if(args["auto"]!="") {
			// auto zoom using specified or default duration
			if(args["duration"]=="") duration = mvmgr->getAutoMoveDuration();
		  		  
			if(args["auto"]=="out") mvmgr->autoZoomOut(duration, 0);
			else if(args["auto"]=="initial") mvmgr->autoZoomOut(duration, 1);
			else if(args["manual"]=="1") {
				mvmgr->autoZoomIn(duration, 1);  // have to explicity allow possible manual zoom 
			} else mvmgr->autoZoomIn(duration, 0);  

		} else if (args["fov"]!="") {
			// zoom to specific field of view
			mvmgr->zoomTo( StelUtils::stringToDouble(args["fov"]), duration);

		} else if (args["delta_fov"]!="") stcore->getProjection()->setFov(stcore->getProjection()->getFov() + StelUtils::stringToDouble(args["delta_fov"]));
		// should we record absolute fov instead of delta? isn't usually smooth playback
		else status = 0;

	} else if(command == "timerate") {   // NOTE: accuracy issue related to frame rate
	  
		if(args["rate"]!="") {
			stcore->getNavigation()->setTimeSpeed(StelUtils::stringToDouble(args["rate"])*JD_SECOND);
		  
		} else if(args["action"]=="pause") {
			FlagTimePause = !FlagTimePause;
			if(FlagTimePause) {
				temp_time_velocity = stcore->getNavigation()->getTimeSpeed();
				stcore->getNavigation()->setTimeSpeed(0);
			} else {
				stcore->getNavigation()->setTimeSpeed(temp_time_velocity);
			}
		  
		} else if(args["action"]=="resume") {
			FlagTimePause = 0;
			stcore->getNavigation()->setTimeSpeed(temp_time_velocity);
		  
		} else if(args["action"]=="increment") {
			// speed up time rate
			double s = stcore->getNavigation()->getTimeSpeed();
			if (s>=JD_SECOND) s*=10.;
			else if (s<-JD_SECOND) s/=10.;
			else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
			else if (s>=-JD_SECOND && s<0.) s=0.;
			stcore->getNavigation()->setTimeSpeed(s);
		  
			// for safest script replay, record as absolute amount
			commandline = "timerate rate " + StelUtils::doubleToString(s/JD_SECOND);

		} else if(args["action"]=="decrement") {
			double s = stcore->getNavigation()->getTimeSpeed();
			if (s>JD_SECOND) s/=10.;
			else if (s<=-JD_SECOND) s*=10.;
			else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
			else if (s>0. && s<=JD_SECOND) s=0.;
			stcore->getNavigation()->setTimeSpeed(s);

			// for safest script replay, record as absolute amount
			commandline = "timerate rate " + StelUtils::doubleToString(s/JD_SECOND);
		} else status=0;
    
	} else if(command == "date") {
		// ISO 8601-like format [[+/-]YYYY-MM-DD]Thh:mm:ss (no timzone offset, T is literal)
		if(args["local"]!="") {
			double jd;
			string new_date;

			if(args["local"][0] == 'T') {
				// set time only (don't change day)
				string sky_date = stapp->getLocaleMgr().get_ISO8601_time_local(stcore->getNavigation()->getJDay());
				new_date = sky_date.substr(0,10) + args["local"];
			} else new_date = args["local"];

			if(string_to_jday( new_date, jd ) ) {
				stcore->getNavigation()->setJDay(jd - stapp->getLocaleMgr().get_GMT_shift(jd) * JD_HOUR);
			} else {
				debug_message = _("Error parsing date.");
				status = 0;
			} 
		  
		} else if(args["utc"]!="") {
			double jd;
			if(string_to_jday( args["utc"], jd ) ) {
				stcore->getNavigation()->setJDay(jd);
			} else {
				debug_message = _("Error parsing date.");
				status = 0;
			}
		  
		} else if(args["relative"]!="") {  // value is a float number of earth days (24 hours)
			double days = StelUtils::stringToDouble(args["relative"]);
			stcore->getNavigation()->setJDay(stcore->getNavigation()->getJDay() + days );

		} else if(args["sidereal"]!="") {  // value is a float number of sidereal days
			double days = StelUtils::stringToDouble(args["sidereal"]);

			const Planet* home = stcore->getObservatory()->getHomePlanet();
			if (home->getEnglishName() != "Solar System Observer")
				days *= home->getSiderealDay();

			stcore->getNavigation()->setJDay(stcore->getNavigation()->getJDay() + days );

		} else if(args["load"]=="current") {
			// set date to current date
			stcore->getNavigation()->setJDay(StelUtils::getJDFromSystem());
		} else if(args["load"]=="preset") {
			// set date to preset (or current) date, based on user setup
			// TODO: should this record as the actual date used?
			if (stcore->getNavigation()->getStartupTimeMode()=="preset" || stcore->getNavigation()->getStartupTimeMode()=="Preset")
				stcore->getNavigation()->setJDay(stcore->getNavigation()->getPresetSkyTime() -
				                stapp->getLocaleMgr().get_GMT_shift(stcore->getNavigation()->getPresetSkyTime()) * JD_HOUR);
			else stcore->getNavigation()->setJDay(StelUtils::getJDFromSystem());

		} else status=0;
	  
	} else if (command == "moveto") {
	  
		if(args["lat"]!="" || args["lon"]!="" || args["alt"]!="") {

			const Observer* observatory = stcore->getObservatory();

			double lat = observatory->get_latitude();
			double lon = observatory->get_longitude();
			double alt = observatory->get_altitude();
			
			string name;
			int delay = 0;
		  
			if(args["name"]!="") name = args["name"];
			if(args["lat"]!="") lat = StelUtils::stringToDouble(args["lat"]);
			if(args["lon"]!="") lon = StelUtils::stringToDouble(args["lon"]);
			if(args["alt"]!="") alt = StelUtils::stringToDouble(args["alt"]);
			
			if (args["delay"]!="") delay = (int)(1000.*StelUtils::stringToDouble(args["duration"]));
		  
			stcore->getObservatory()->moveTo(lat,lon,alt,delay,StelUtils::stringToWstring(name));
		} else status = 0;

	} else if(command=="image") {

		ImageMgr* script_images = (ImageMgr*)StelApp::getInstance().getModuleMgr().getModule("ImageMgr");

		if(args["name"]=="") {
			debug_message = _("Image name required.");
			status = 0;
		} else if(args["action"]=="drop") {
			script_images->drop_image(args["name"]);
		} else {
			if(args["action"]=="load" && args["filename"]!="") {
				Image::IMAGE_POSITIONING img_pos = Image::POS_VIEWPORT;
				if(args["coordinate_system"] == "horizontal") img_pos = Image::POS_HORIZONTAL;
				else if(args["coordinate_system"] == "equatorial") img_pos = Image::POS_EQUATORIAL;
				else if(args["coordinate_system"] == "j2000") img_pos = Image::POS_J2000;

				string image_filename;
				if(scripts->is_playing())
					image_filename = scripts->get_script_path() + args["filename"];
				else
					image_filename = "data/" + args["filename"];
					
				try
				{
					image_filename = stapp->getFileMgr().findFile(image_filename);
					script_images->load_image(image_filename, args["name"], img_pos);
				}
				catch(exception& e)
				{
					cerr << "ERROR finding script: " << e.what() << endl;
					debug_message = _("Unable to open file: ") + StelUtils::stringToWstring(image_filename);
				}				  
			}

			if( status ) {
				Image * img = script_images->get_image(args["name"]);
			  
				if(img != NULL) {
					double duration=0.0;
					if (args["duration"]!="") duration = StelUtils::stringToDouble(args["duration"]);
					if(args["alpha"]!="") img->set_alpha(StelUtils::stringToDouble(args["alpha"]), duration);
					if(args["scale"]!="") img->set_scale(StelUtils::stringToDouble(args["scale"]), duration);
					if(args["rotation"]!="") img->set_rotation(StelUtils::stringToDouble(args["rotation"]), duration);
					if(args["xpos"]!="" || args["ypos"]!="")
						img->set_location(StelUtils::stringToDouble(args["xpos"]), args["xpos"]!="",
							StelUtils::stringToDouble(args["ypos"]), args["ypos"]!="",
							duration);
					// for more human readable scripts, as long as someone doesn't do both...
					if(args["altitude"]!="" || args["azimuth"]!="") 
						img->set_location(StelUtils::stringToDouble(args["altitude"]), args["altitude"]!="",
										  StelUtils::stringToDouble(args["azimuth"]), args["azimuth"]!="", duration);
				} else {
					debug_message = _("Unable to find image: ") + StelUtils::stringToWstring(args["name"]);
					status=0;
				}
			}
		}
	} 
	else if(command=="audio") {

#ifndef HAVE_SDL_SDL_MIXER_H
		debug_message = _("This executable was compiled without audio support.");
		status = 0;
#else
  
		if(args["action"]=="drop") {
			if(audio) delete audio;
			audio = NULL;

		} else if(args["action"]=="sync") {
			if(audio) audio->sync();

		} else if(args["action"]=="pause") {
			if(audio) audio->pause();

		} else if(args["action"]=="resume") {
			if(audio) audio->resume();

		} else if( args["action"]=="play" && args["filename"]!="") {
			// only one track at a time allowed
			if(audio) delete audio;
		  
			if(scripts->is_playing())
			{
				// If we're playing the audio file from a script, search for the audio file
				// in the script directory where the script file exists.
				audio = new Audio(scripts->get_script_path() + args["filename"],
						"default track",
      						StelUtils::stringToLong(args["output_rate"]));
				audio->play(args["loop"]=="on");
			}
			else
			{
				// If we're not playing a script at the moment use the file manager to 
				// search for it.  This way we can accept full paths or relative paths 
				// from the pwd and so on.
				try
				{
					string audioFilePath = StelApp::getInstance().getFileMgr().findFile(args["filename"], StelFileMgr::FILE);
					audio = new Audio(audioFilePath, "default track", StelUtils::stringToLong(args["output_rate"]));
					audio->play(args["loop"]=="on");
				}
				catch(exception& e)
				{
					cerr << "ERROR while trying to play audio file: " << args["filename"] << " : " << e.what() << endl;
				}
			}

			// if fast forwarding mute (pause) audio
			if(stapp->getTimeMultiplier()!=1) audio->pause();		  

		} else if(args["volume"]!="") {

			recordable = 0;
			if(audio!=NULL) {
				if(args["volume"] == "increment") {
					audio->increment_volume();
				} else if(args["volume"] == "decrement") {
					audio->decrement_volume();
				} else audio->set_volume( StelUtils::stringToDouble(args["volume"]) );
			}
		} else status = 0;
#endif 
	}
	else if(command=="script") {

		ImageMgr* script_images = (ImageMgr*)StelApp::getInstance().getModuleMgr().getModule("ImageMgr");

		if(args["action"]=="end") {
			// stop script, audio, and unload any loaded images
			if(audio) {
				delete audio;
				audio = NULL;
			}
			scripts->cancel_script();
			script_images->drop_all_images();

		} else if(args["action"]=="play" && args["filename"]!="") {
			if(scripts->is_playing()) {

				string script_path = scripts->get_script_path();

				// stop script, audio, and unload any loaded images
				if(audio) {
					delete audio;
					audio = NULL;
				}
				scripts->cancel_script();
				script_images->drop_all_images();

				// keep same script path 
				scripts->play_script(script_path + args["filename"], script_path);
			} else {
				scripts->play_script(args["path"] + args["filename"], args["path"]);
			}

		} else if(args["action"]=="record") {
			scripts->record_script(args["filename"]);
			recordable = 0;  // don't record this command!
		} else if(args["action"]=="cancelrecord") {
			scripts->cancel_record_script();
			recordable = 0;  // don't record this command!
		} else if(args["action"]=="pause" && !scripts->is_paused()) {
			// n.b. action=pause TOGGLES pause
			if(audio) audio->pause();
			scripts->pause_script();
		} else if (args["action"]=="pause" || args["action"]=="resume") {
			scripts->resume_script();
			if(audio) audio->sync();
		} else status =0;

	} else if(command=="clear") {

		// TODO move to stelcore

		// set sky to known, standard states (used by scripts for simplicity)
		execute_command("set home_planet Earth");

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
		args["path"] = scripts->get_script_path();
		lmgr->loadLandscape(args);
	  
	} else if(command=="meteors") {
		if(args["zhr"]!="") {
			metmgr->setZHR(StelUtils::stringToInt(args["zhr"]));
		} else status =0;

	} else if(command=="configuration") {

		cout << "\"configuration\" command no longer supported.\n";
		status = 0;

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
		// 				  stcore->navigation->set_JDay(getJDFromSystem());
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
	} else if(command=="screenshot") {
		if (!scripts->can_write_files())
		{
			debug_message = _("Scripting cannot write files (check setting files:scripts_can_write_files)");
			status = 0;	
		}
		else
		{
			if(args["prefix"]=="")
			{
				debug_message = _("mandatory argument \"prefix\" not specified");
				status = 0;
			}
			else
				{
					StelApp::getInstance().saveScreenShot(args["prefix"].c_str(), args["dir"].c_str());
				status = 1;

			}
		}
	} else {
		debug_message = _("Unrecognized or malformed command name.");
		status = 0;
	}

	if(status ) {

		// if recording commands, do that now
		if(recordable) scripts->record_command(commandline);

		//    cout << commandline << endl;

	} else {

		// Show gui error window only if script asked for gui debugging
		if(scripts->is_playing() && scripts->get_gui_debug())
		{
			StelUI* ui = (StelUI*)StelApp::getInstance().getModuleMgr().getModule("StelUI");
			ui->show_message(_("Could not execute command:") + wstring(L"\n\"") + 
									StelUtils::stringToWstring(commandline) + wstring(L"\"\n\n") + debug_message, 7000);
		}	
		cerr << "Could not execute: " << commandline << endl << StelUtils::wstringToString(debug_message) << endl;
	}

	return(status);

}


// set flags
// if caller is not trusted, some flags can't be changed 
// newval is new value of flag changed

int StelCommandInterface::set_flag(string name, string value, bool &newval, bool trusted) {

	bool status = 1; 

	// value can be "on", "off", or "toggle"
	if(value == "toggle") {

		if(trusted) {

			/* disabled due to code rework

			// normal scripts shouldn't be able to change these user settings
			if(name=="enable_zoom_keys") { 
				newval = !stcore->getFlagEnableZoomKeys();
				stcore->setFlagEnableZoomKeys(newval); }
			else if(name=="enable_move_keys") {
				newval = !stcore->getFlagEnableMoveKeys();
				stcore->setFlagEnableMoveKeys(newval); }
			else if(name=="enable_move_mouse") newval = (stapp->FlagEnableMoveMouse = !stapp->FlagEnableMoveMouse);
			else if(name=="menu") newval = (ui->FlagMenu = !ui->FlagMenu);
			else if(name=="help") newval = (ui->FlagHelp = !ui->FlagHelp);
			else if(name=="infos") newval = (ui->FlagInfos = !ui->FlagInfos);
			else if(name=="show_topbar") newval = (ui->FlagShowTopBar = !ui->FlagShowTopBar);
			else if(name=="show_time") newval = (ui->FlagShowTime = !ui->FlagShowTime);
			else if(name=="show_date") newval = (ui->FlagShowDate = !ui->FlagShowDate);
			else if(name=="show_appname") newval = (ui->FlagShowAppName = !ui->FlagShowAppName);
			else if(name=="show_fps") newval = (ui->FlagShowFps = !ui->FlagShowFps);
			else if(name=="show_fov") newval = (ui->FlagShowFov = !ui->FlagShowFov);
			else if(name=="enable_tui_menu") newval = (ui->FlagEnableTuiMenu = !ui->FlagEnableTuiMenu);
			else if(name=="show_gravity_ui") newval = (ui->FlagShowGravityUi = !ui->FlagShowGravityUi);
			else if(name=="gravity_labels") {
				newval = !stcore->getFlagGravityLabels();
				stcore->setFlagGravityLabels(newval);
				}
			else status = 0;  // no match here, anyway
			*/
			status = 0;

		} else status = 0;

		ConstellationMgr* cmgr = (ConstellationMgr*)StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr");
		StarMgr* smgr = (StarMgr*)StelApp::getInstance().getModuleMgr().getModule("StarMgr");
		NebulaMgr* nmgr = (NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr");
		SolarSystem* ssmgr = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
		LandscapeMgr* lmgr = (LandscapeMgr*)StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr");
		GridLinesMgr* grlmgr = (GridLinesMgr*)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");
		MovementMgr* mvmgr = (MovementMgr*)StelApp::getInstance().getModuleMgr().getModule("MovementMgr");
		StelUI* ui = (StelUI*)StelApp::getInstance().getModuleMgr().getModule("StelUI");
		ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("ScriptMgr");
		
		if(name=="constellation_drawing") {
			newval = !cmgr->getFlagLines();
			cmgr->setFlagLines(newval);
		} 
		else if(name=="constellation_names") {
			newval = !cmgr->getFlagNames();
			cmgr->setFlagNames(newval);
		} 
		else if(name=="constellation_art") {
			newval = !cmgr->getFlagArt();
			cmgr->setFlagArt(newval);
		} 
		else if(name=="constellation_boundaries") {
			newval = !cmgr->getFlagBoundaries();
			cmgr->setFlagBoundaries(newval);
		} 
		else if(name=="constellation_pick") { 
             newval = !cmgr->getFlagIsolateSelected();
             cmgr->setFlagIsolateSelected(newval);
        }
		else if(name=="star_twinkle") {
			newval = !smgr->getFlagTwinkle();
			smgr->setFlagTwinkle(newval);
		}
		else if(name=="point_star") {
			newval = !smgr->getFlagPointStar();
			smgr->setFlagPointStar(newval);
		}
		else if(name=="show_selected_object_info") newval = (ui->FlagShowSelectedObjectInfo = !ui->FlagShowSelectedObjectInfo);
		else if(name=="show_tui_datetime") newval = (ui->FlagShowTuiDateTime = !ui->FlagShowTuiDateTime);
		else if(name=="show_tui_short_obj_info") newval = (ui->FlagShowTuiShortObjInfo = !ui->FlagShowTuiShortObjInfo);
		else if(name=="manual_zoom") {
			newval = !mvmgr->getFlagManualAutoZoom();  
			mvmgr->setFlagManualAutoZoom(newval); }
		else if(name=="show_script_bar") newval = (ui->FlagShowScriptBar = !ui->FlagShowScriptBar);
		else if(name=="fog") {
			newval = !lmgr->getFlagFog();
			lmgr->setFlagFog(newval); 
		}
		else if(name=="atmosphere") {
			newval = !lmgr->getFlagAtmosphere();
			lmgr->setFlagAtmosphere(newval);
			if(!newval) lmgr->setFlagFog(false);  // turn off fog with atmosphere
		}
		/*		else if(name=="chart") {
			newval = !stapp->getVisionModeChart();
			if (newval) stapp->setVisionModeChart();
		}
		else if(name=="night") {
			newval = !stapp->getVisionModeNight();
			if (newval) stapp->setVisionModeNight();
		}
		*/
		//else if(name=="use_common_names") newval = (stcore->FlagUseCommonNames = !stcore->FlagUseCommonNames);
		else if(name=="azimuthal_grid") {
		newval = !grlmgr->getFlagAzimutalGrid();
		grlmgr->setFlagAzimutalGrid(newval);
		}
		else if(name=="equatorial_grid") {
		newval = !grlmgr->getFlagEquatorGrid();
		grlmgr->setFlagEquatorGrid(newval);
		}
		else if(name=="equator_line") {
		newval = !grlmgr->getFlagEquatorLine();
		grlmgr->setFlagEquatorLine(newval);
		}
		else if(name=="ecliptic_line") {
		newval = !grlmgr->getFlagEclipticLine();
		grlmgr->setFlagEclipticLine(newval);
		}
		else if(name=="meridian_line") {
		newval = !grlmgr->getFlagMeridianLine();
		 grlmgr->setFlagMeridianLine(newval);
		}
		else if(name=="cardinal_points") {
			newval = !lmgr->getFlagCardinalsPoints();
			lmgr->setFlagCardinalsPoints(newval);
		}
		else if(name=="moon_scaled") {
			newval = !ssmgr->getFlagMoonScale();
			ssmgr->setFlagMoonScale(newval);
		}
		else if(name=="landscape") {
			newval = !lmgr->getFlagLandscape();
			lmgr->setFlagLandscape(newval);
			}
		else if(name=="stars") {
			newval = !smgr->getFlagStars();
			smgr->setFlagStars(newval);
		}
		else if(name=="star_names") {
			newval = !smgr->getFlagNames();
			smgr->setFlagNames(newval);
		}
		else if(name=="planets") {
			newval = !ssmgr->getFlagPlanets();
			ssmgr->setFlagPlanets(newval);
			if (!ssmgr->getFlagPlanets()) ssmgr->setFlagHints(false);
		}
		else if(name=="planet_names") {
			newval = !ssmgr->getFlagHints();
			ssmgr->setFlagHints(newval);
			if(ssmgr->getFlagHints()) ssmgr->setFlagPlanets(true);  // for safety if script turns planets off
		}
		else if(name=="planet_orbits") {
		newval = !ssmgr->getFlagOrbits();
		ssmgr->setFlagOrbits(newval);
		}
		else if(name=="nebulae") {
			newval = !nmgr->getFlagShow();
			nmgr->setFlagShow(newval);
			}
		else if(name=="nebula_names") {
			newval = !nmgr->getFlagHints();
			if(newval) nmgr->setFlagShow(true);  // make sure visible
			nmgr->setFlagHints(newval);
		}
		else if(name=="milky_way") {
			MilkyWay* mw = (MilkyWay*)stapp->getModuleMgr().getModule("MilkyWay");
			newval = !mw->getFlagShow();
			mw->setFlagShow(newval);
		}
		else if(name=="bright_nebulae") {
			newval = !nmgr->getFlagBright();
			nmgr->setFlagBright(newval);
			}
		else if(name=="object_trails")
		{
		newval = !ssmgr->getFlagTrails();
		ssmgr->setFlagTrails(newval);
		}
		else if(name=="track_object") {
			newval = !mvmgr->getFlagTracking();
			mvmgr->setFlagTracking(newval);
		}
		else if(name=="script_gui_debug") {  // Not written to config - script specific
			newval = !scripts->get_gui_debug();
			scripts->set_gui_debug(newval);
		}
		else if(name=="landscape_sets_location")
		{
			newval = !lmgr->getFlagLandscapeSetsLocation();
			lmgr->setFlagLandscapeSetsLocation(newval);
		}
		else return(status);  // no matching flag found untrusted, but maybe trusted matched

	} else {

		newval = (value == "on" || value == "1");

		if(trusted) {

			/* disabled due to code rework
			// normal scripts shouldn't be able to change these user settings
			if(name=="enable_zoom_keys") stcore->setFlagEnableZoomKeys(newval);
			else if(name=="enable_move_keys") stcore->setFlagEnableMoveKeys(newval);
			else if(name=="enable_move_mouse") stapp->FlagEnableMoveMouse = newval;
			else if(name=="menu") ui->FlagMenu = newval;
			else if(name=="help") ui->FlagHelp = newval;
			else if(name=="infos") ui->FlagInfos = newval;
			else if(name=="show_topbar") ui->FlagShowTopBar = newval;
			else if(name=="show_time") ui->FlagShowTime = newval;
			else if(name=="show_date") ui->FlagShowDate = newval;
			else if(name=="show_appname") ui->FlagShowAppName = newval;
			else if(name=="show_fps") ui->FlagShowFps = newval;
			else if(name=="show_fov") ui->FlagShowFov = newval;
			else if(name=="enable_tui_menu") ui->FlagEnableTuiMenu = newval;
			else if(name=="show_gravity_ui") ui->FlagShowGravityUi = newval;
			else if(name=="gravity_labels") stcore->setFlagGravityLabels(newval);
			else status = 0;

			*/
			status = 0;
		
		} else status = 0;

		ConstellationMgr* cmgr = (ConstellationMgr*)StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr");
		StarMgr* smgr = (StarMgr*)StelApp::getInstance().getModuleMgr().getModule("StarMgr");
		NebulaMgr* nmgr = (NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr");
		SolarSystem* ssmgr = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
		LandscapeMgr* lmgr = (LandscapeMgr*)StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr");
		GridLinesMgr* grlmgr = (GridLinesMgr*)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");
		MovementMgr* mvmgr = (MovementMgr*)StelApp::getInstance().getModuleMgr().getModule("MovementMgr");
		StelUI* ui = (StelUI*)StelApp::getInstance().getModuleMgr().getModule("StelUI");
		ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("ScriptMgr");
		
		if(name=="constellation_drawing") cmgr->setFlagLines(newval);
		else if(name=="constellation_names") cmgr->setFlagNames(newval);
		else if(name=="constellation_art") cmgr->setFlagArt(newval);
		else if(name=="constellation_boundaries") cmgr->setFlagBoundaries(newval);
     	else if(name=="constellation_pick") cmgr->setFlagIsolateSelected(newval);
		else if(name=="star_twinkle") smgr->setFlagTwinkle(newval);
		else if(name=="point_star") smgr->setFlagPointStar(newval);
		else if(name=="show_selected_object_info") ui->FlagShowSelectedObjectInfo = newval;
		else if(name=="show_tui_datetime") ui->FlagShowTuiDateTime = newval;
		else if(name=="show_tui_short_obj_info") ui->FlagShowTuiShortObjInfo = newval;
		else if(name=="manual_zoom") mvmgr->setFlagManualAutoZoom(newval);
		else if(name=="show_script_bar") ui->FlagShowScriptBar = newval;
		else if(name=="fog") lmgr->setFlagFog(newval);
		else if(name=="atmosphere") { 
			lmgr->setFlagAtmosphere ( newval);
			if(!newval) lmgr->setFlagFog(false);  // turn off fog with atmosphere
		}
		else if(name=="azimuthal_grid") grlmgr->setFlagAzimutalGrid(newval);
		else if(name=="equatorial_grid") grlmgr->setFlagEquatorGrid(newval);
		else if(name=="equator_line") grlmgr->setFlagEquatorLine(newval);
		else if(name=="ecliptic_line") grlmgr->setFlagEclipticLine(newval);
		else if(name=="meridian_line") grlmgr->setFlagMeridianLine(newval);
		else if(name=="cardinal_points") lmgr->setFlagCardinalsPoints(newval);
		else if(name=="moon_scaled") ssmgr->setFlagMoonScale(newval);
		else if(name=="landscape") lmgr->setFlagLandscape(newval);
		else if(name=="stars") smgr->setFlagStars(newval);
		else if(name=="star_names") smgr->setFlagNames(newval);
		else if(name=="planets") {
			ssmgr->setFlagPlanets(newval);
			if (!ssmgr->getFlagPlanets()) ssmgr->setFlagHints(false);
		}
		else if(name=="planet_names") {
			ssmgr->setFlagHints(newval);
			if(ssmgr->getFlagHints()) ssmgr->setFlagPlanets(true);  // for safety if script turns planets off
		}
		else if(name=="planet_orbits") ssmgr->setFlagOrbits(newval);
		else if(name=="nebulae") nmgr->setFlagShow(newval);
		else if(name=="nebula_names") {
			nmgr->setFlagShow(true);  // make sure visible
			nmgr->setFlagHints(newval);
		}
		else if(name=="milky_way")
		{
			MilkyWay* mw = (MilkyWay*)stapp->getModuleMgr().getModule("MilkyWay");
			mw->setFlagShow(newval);
		}
		else if(name=="bright_nebulae") nmgr->setFlagBright(newval);
		else if(name=="object_trails") ssmgr->setFlagTrails(newval);
		else if(name=="track_object") mvmgr->setFlagTracking(newval);
		else if(name=="script_gui_debug") scripts->set_gui_debug(newval); // Not written to config - script specific
		else if(name=="landscape_sets_location") lmgr->setFlagLandscapeSetsLocation(newval);
		else return(status);

	}


	return(1);  // flag was found and updated

}


void StelCommandInterface::update(double delta_time)
{
	ScriptMgr* scripts = (ScriptMgr*)StelApp::getInstance().getModuleMgr().getModule("ScriptMgr");
	// keep audio position updated if changing time multiplier
	if (scripts->is_paused())
		return;
	if (audio)
		audio->update(delta_time);
}
