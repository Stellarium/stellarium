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

#ifndef _AUDIO_HPP_
#define _AUDIO_HPP_

#include <config.h>
#include <QString>

#ifdef HAVE_SDL_SDL_MIXER_H
#include <SDL/SDL_mixer.h>

class Audio
{
public:
	Audio(const QString& filename, const QString& name, long int outputRate);
	virtual ~Audio();
	void play(bool loop);
	void sync();  // reset audio track time offset to elapsed time
	void pause();
	void resume();
	void stop();
	void setVolume(float _volume);
	void incrementVolume();
	void decrementVolume();
	QString getName() { return trackName; }
	void update(double deltaTime);

private:
	Mix_Music *track;
	QString trackName;
	bool isPlaying;
	static float masterVolume;  // audio level
	double elapsedSeconds;      // current offset into the track
};
#else
// SDL_mixer is not available - no audio
class Audio
{
public:
	Audio(const QString& filename, const QString& name) {}
	virtual ~Audio() {}
	void play(bool loop) {}
	void pause() {}
	void sync() {}
	void resume() {}
	void stop() {}
	void setVolume(float _volume) {}
	void incrementVolume() {}
	void decrementVolume() {}
	QString getName() { return "Compiled with no Audio!"; }
	void update(double deltaTime) {}

private:
};
#endif

#endif // _AUDIO_HPP_
