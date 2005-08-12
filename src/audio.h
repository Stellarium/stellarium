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

// manage an audio track

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <config.h>

#include <string>
#include "SDL.h"

#ifdef HAVE_SDL_MIXER_H
# include "SDL_mixer.h"

class Audio
{
 public:
  Audio(std::string filename, std::string name, long int output_rate);
  virtual ~Audio();
  void play(bool loop);
  void sync();  // reset audio track time offset to elapsed time
  void pause();
  void resume();
  void stop();
  void set_volume(float _volume);
  void increment_volume();
  void decrement_volume();
  std::string get_name() { return track_name; };
  void update(int delta_time);

 private:
  Mix_Music *track;
  std::string track_name;
  bool is_playing;
  static float master_volume;  // audio level
  double elapsed_seconds;  // current offset into the track
};
#else
// SDL_mixer is not available - no audio
class Audio
{
 public:
  Audio(std::string filename, std::string name) {;}
  virtual ~Audio() {;}
  void play(bool loop) {;}
  void pause() {;}
  void resync() {;}
  void resume() {;}
  void stop() {;}
  void set_volume(float _volume) {;}
  void increment_volume() {;}
  void decrement_volume() {;}
  std::string get_name() { return "Compiled with no Audio!"; };
  void update(int delta_time) {;}

 private:
};
#endif

#endif // _AUDIO_H
