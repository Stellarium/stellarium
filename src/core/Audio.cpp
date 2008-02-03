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
#include <config.h>

#include "Audio.hpp"
#include "Translator.hpp"

#ifdef HAVE_SDL_SDL_MIXER_H

float Audio::master_volume = 0.5;

Audio::Audio(std::string filename, std::string name, long int output_rate) {

	if(output_rate < 1000) output_rate = 22050;

	// initialize audio
	if(Mix_OpenAudio(output_rate, MIX_DEFAULT_FORMAT, 2, 4096)) {
		printf("Unable to open audio output!\n");
		track = NULL;
		return;
	}

	Mix_VolumeMusic(int(MIX_MAX_VOLUME*master_volume));

	track = Mix_LoadMUS(filename.c_str());
	if(track == NULL) {
		is_playing = 0;
		std::cout << "Could not load audio file " << filename << "\n";
	} else is_playing = 1;

	track_name = name;
}

Audio::~Audio() {
	Mix_HaltMusic(); // stop playing
	Mix_FreeMusic(track);  // free memory

	// stop audio use
	Mix_CloseAudio();
}

void Audio::play(bool loop) {
	
	// TODO check for load errors

	if(track) {
		if(loop) Mix_PlayMusic(track, -1);
		else Mix_PlayMusic(track, 0);
		is_playing = 1;
		elapsed_seconds = 0;
		//		std::cout << "now playing audio\n";
	}
}

// used solely to track elapsed seconds of play
void Audio::update(double delta_time) {
	
	if(track) elapsed_seconds += delta_time/1000.f;

}

// sychronize with elapsed time
void Audio::sync() {
	if(track==NULL) return;

	Mix_SetMusicPosition(elapsed_seconds);  // ISSUE doesn't work for all audio formats
	Mix_ResumeMusic();
	is_playing = 1;

	// printf("Synced audio to %f seconds\n", elapsed_seconds);
}

void Audio::pause() {
	Mix_PauseMusic();
	is_playing=0;
}

void Audio::resume() {
	Mix_ResumeMusic();
	is_playing=1;
}

void Audio::stop() {
	Mix_HaltMusic();
	is_playing=0;
}

// _volume should be between 0 and 1
void Audio::set_volume(float _volume) {
	if(_volume >= 0 && _volume <= 1) {
		master_volume = _volume;
		Mix_VolumeMusic(int(MIX_MAX_VOLUME*master_volume));
	}
}

void Audio::increment_volume() {
	master_volume += 0.1f;  // 10%

	if(master_volume > 1) master_volume = 1;

	Mix_VolumeMusic(int(MIX_MAX_VOLUME*master_volume));

	// printf("volume %f\n", master_volume);
}

void Audio::decrement_volume() {
	master_volume -= 0.1f;  // 10%

	if(master_volume < 0) master_volume = 0;

	Mix_VolumeMusic(int(MIX_MAX_VOLUME*master_volume));

	//	printf("volume %f\n", master_volume);
}

#else
// SDL_mixer is not available - no audio
#endif
