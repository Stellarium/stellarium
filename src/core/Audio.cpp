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

#include <config.h>
#include <QString>
#include <QDebug>

#include "Audio.hpp"
#include "Translator.hpp"

#ifdef HAVE_SDL_SDL_MIXER_H

float Audio::masterVolume = 0.5;

Audio::Audio(const QString& filename, const QString& name, long int outputRate) {
	if(outputRate < 1000) outputRate = 22050;

	// initialize audio
	if(Mix_OpenAudio(outputRate, MIX_DEFAULT_FORMAT, 2, 4096)) {
		qWarning() << "ERROR - Unable to open initialize audio output system";
		track = NULL;
		return;
	}

	Mix_VolumeMusic(int(MIX_MAX_VOLUME*masterVolume));

	track = Mix_LoadMUS(filename.toLocal8Bit());
	if(track == NULL) {
		isPlaying = 0;
		qWarning() << "ERRIR - Could not load audio file: " << filename;
	} else 
		isPlaying = 1;

	trackName = name;
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
		isPlaying = 1;
		elapsedSeconds = 0;
		// qDebug() << "now playing audio";
	}
}

// used solely to track elapsed seconds of play
void Audio::update(double deltaTime) {
	
	if(track) elapsedSeconds += deltaTime/1000.f;

}

// sychronize with elapsed time
void Audio::sync() {
	if(track==NULL) return;

	Mix_SetMusicPosition(elapsedSeconds);  // ISSUE doesn't work for all audio formats
	Mix_ResumeMusic();
	isPlaying = 1;

	// qDebug("Synced audio to %f seconds\n", elapsedSeconds);
}

void Audio::pause() {
	Mix_PauseMusic();
	isPlaying=0;
}

void Audio::resume() {
	Mix_ResumeMusic();
	isPlaying=1;
}

void Audio::stop() {
	Mix_HaltMusic();
	isPlaying=0;
}

// _volume should be between 0 and 1
void Audio::setVolume(float _volume) {
	if(_volume >= 0 && _volume <= 1) {
		masterVolume = _volume;
		Mix_VolumeMusic(int(MIX_MAX_VOLUME*masterVolume));
	}
}

void Audio::incrementVolume() {
	masterVolume += 0.1f;  // 10%

	if(masterVolume > 1) masterVolume = 1;

	Mix_VolumeMusic(int(MIX_MAX_VOLUME*masterVolume));

	// qDebug("volume %f\n", masterVolume);
}

void Audio::decrementVolume() {
	masterVolume -= 0.1f;  // 10%

	if(masterVolume < 0) masterVolume = 0;

	Mix_VolumeMusic(int(MIX_MAX_VOLUME*masterVolume));

	// qDebug("volume %f\n", masterVolume);
}

#else
// SDL_mixer is not available - no audio
#endif
