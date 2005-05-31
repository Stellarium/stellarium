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

#ifdef HAVE_SDL_MIXER_H
Audio::Audio(std::string filename, std::string name) {
    // audio parameters (could be set as parameters to this constructor)
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16SYS; /* 16-bit stereo */  // TODO: cross platform issues?
    int audio_channels = 2;
    int audio_buffers = 4096;

	// initialize audio
	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		printf("Unable to open audio!\n");
		return;
		// TODO: how to test this case?
	}

	// set the music volume to 1/2 maximum
	Mix_VolumeMusic(MIX_MAX_VOLUME/2);

	track = Mix_LoadMUS(filename.c_str());
	if(track == NULL) 	std::cout << "Could not load audio file " << filename << "\n";
	track_name = name;
}

Audio::~Audio() {
  Mix_HaltMusic(); // stop playing
  Mix_FreeMusic(track);  // free memory

  // stop audio use
  Mix_CloseAudio();
}

void Audio::play(bool loop) {
	
	if(loop) Mix_PlayMusic(track, -1);
	else Mix_PlayMusic(track, 0);
	std::cout << "now playing audio\n";

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

// _volume should be between 0 and 1
void Audio::set_volume(float _volume) {
	Mix_VolumeMusic(int(_volume*MIX_MAX_VOLUME));

}

void Audio::increment_volume() {
	int vol = Mix_VolumeMusic(-1);
	
	int step = int(0.1f * MIX_MAX_VOLUME);
	if(!step) step = 1;
	Mix_VolumeMusic(vol+step);

	// printf("volume %d + %d\n", vol, step);
}

void Audio::decrement_volume() {
	int vol = Mix_VolumeMusic(-1);
	
	int step = int(-0.1f * MIX_MAX_VOLUME);
	if(!step) step = -1;
	if(-1*step>vol) vol = step = 0;

	Mix_VolumeMusic(vol+step);

	//	printf("volume %d - %d\n", vol, step);
}

#else
// SDL_mixer is not available - no audio
#endif
