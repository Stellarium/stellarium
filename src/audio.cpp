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

// manage an audio track (SDL mixer music track)

#include <iostream>
#include "audio.h"

Audio::Audio(std::string filename, std::string name) {
  track = Mix_LoadMUS(filename.c_str());
  track_name = name;
}

Audio::~Audio() {
  Mix_HaltMusic(); // stop playing
  Mix_FreeMusic(track);  // free memory
}

void Audio::play(bool loop) {
  std::cout << "now playing music\n";
  if(loop) Mix_PlayMusic(track, -1);
  else Mix_PlayMusic(track, 0);
}

void Audio::pause() {
  Mix_PauseMusic();
}

void Audio::resume() {
  Mix_ResumeMusic();
}

void Audio::stop() {
  Mix_HaltMusic();
}

