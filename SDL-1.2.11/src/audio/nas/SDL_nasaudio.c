/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This driver was written by:
    Erik Inge Bolsø
    knan@mo.himolde.no
*/
#include "SDL_config.h"

/* Allow access to a raw mixing buffer */

#include <signal.h>
#include <unistd.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_nasaudio.h"

/* The tag name used by artsc audio */
#define NAS_DRIVER_NAME         "nas"

static struct SDL_PrivateAudioData *this2 = NULL;

/* Audio driver functions */
static int NAS_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void NAS_WaitAudio(_THIS);
static void NAS_PlayAudio(_THIS);
static Uint8 *NAS_GetAudioBuf(_THIS);
static void NAS_CloseAudio(_THIS);

/* Audio driver bootstrap functions */

static int Audio_Available(void)
{
	AuServer *aud = AuOpenServer("", 0, NULL, 0, NULL, NULL);
	if (!aud) return 0;

	AuCloseServer(aud);
	return 1;
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));

	/* Set the function pointers */
	this->OpenAudio = NAS_OpenAudio;
	this->WaitAudio = NAS_WaitAudio;
	this->PlayAudio = NAS_PlayAudio;
	this->GetAudioBuf = NAS_GetAudioBuf;
	this->CloseAudio = NAS_CloseAudio;

	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap NAS_bootstrap = {
	NAS_DRIVER_NAME, "Network Audio System",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void NAS_WaitAudio(_THIS)
{
	while ( this->hidden->buf_free < this->hidden->mixlen ) {
		AuEvent ev;
		AuNextEvent(this->hidden->aud, AuTrue, &ev);
		AuDispatchEvent(this->hidden->aud, &ev);
	}
}

static void NAS_PlayAudio(_THIS)
{
	while (this->hidden->mixlen > this->hidden->buf_free) { /* We think the buffer is full? Yikes! Ask the server for events,
				    in the hope that some of them is LowWater events telling us more
				    of the buffer is free now than what we think. */
		AuEvent ev;
		AuNextEvent(this->hidden->aud, AuTrue, &ev);
		AuDispatchEvent(this->hidden->aud, &ev);
	}
	this->hidden->buf_free -= this->hidden->mixlen;

	/* Write the audio data */
	AuWriteElement(this->hidden->aud, this->hidden->flow, 0, this->hidden->mixlen, this->hidden->mixbuf, AuFalse, NULL);

	this->hidden->written += this->hidden->mixlen;
	
#ifdef DEBUG_AUDIO
	fprintf(stderr, "Wrote %d bytes of audio data\n", this->hidden->mixlen);
#endif
}

static Uint8 *NAS_GetAudioBuf(_THIS)
{
	return(this->hidden->mixbuf);
}

static void NAS_CloseAudio(_THIS)
{
	if ( this->hidden->mixbuf != NULL ) {
		SDL_FreeAudioMem(this->hidden->mixbuf);
		this->hidden->mixbuf = NULL;
	}
	if ( this->hidden->aud ) {
		AuCloseServer(this->hidden->aud);
		this->hidden->aud = 0;
	}
}

static unsigned char sdlformat_to_auformat(unsigned int fmt)
{
  switch (fmt)
    {
    case AUDIO_U8:
      return AuFormatLinearUnsigned8;
    case AUDIO_S8:
      return AuFormatLinearSigned8;
    case AUDIO_U16LSB:
      return AuFormatLinearUnsigned16LSB;
    case AUDIO_U16MSB:
      return AuFormatLinearUnsigned16MSB;
    case AUDIO_S16LSB:
      return AuFormatLinearSigned16LSB;
    case AUDIO_S16MSB:
      return AuFormatLinearSigned16MSB;
    }
  return AuNone;
}

static AuBool
event_handler(AuServer* aud, AuEvent* ev, AuEventHandlerRec* hnd)
{
	switch (ev->type) {
	case AuEventTypeElementNotify: {
		AuElementNotifyEvent* event = (AuElementNotifyEvent *)ev;

		switch (event->kind) {
		case AuElementNotifyKindLowWater:
			if (this2->buf_free >= 0) {
				this2->really += event->num_bytes;
				gettimeofday(&this2->last_tv, 0);
				this2->buf_free += event->num_bytes;
			} else {
				this2->buf_free = event->num_bytes;
			}
			break;
		case AuElementNotifyKindState:
			switch (event->cur_state) {
			case AuStatePause:
				if (event->reason != AuReasonUser) {
					if (this2->buf_free >= 0) {
						this2->really += event->num_bytes;
						gettimeofday(&this2->last_tv, 0);
						this2->buf_free += event->num_bytes;
					} else {
						this2->buf_free = event->num_bytes;
					}
				}
				break;
			}
		}
	}
	}
	return AuTrue;
}

static AuDeviceID
find_device(_THIS, int nch)
{
	int i;
	for (i = 0; i < AuServerNumDevices(this->hidden->aud); i++) {
		if ((AuDeviceKind(AuServerDevice(this->hidden->aud, i)) ==
				AuComponentKindPhysicalOutput) &&
			AuDeviceNumTracks(AuServerDevice(this->hidden->aud, i)) == nch) {
			return AuDeviceIdentifier(AuServerDevice(this->hidden->aud, i));
		}
	}
	return AuNone;
}

static int NAS_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	AuElement elms[3];
	int buffer_size;
	Uint16 test_format, format;

	this->hidden->mixbuf = NULL;

	/* Try for a closest match on audio format */
	format = 0;
	for ( test_format = SDL_FirstAudioFormat(spec->format);
						! format && test_format; ) {
		format = sdlformat_to_auformat(test_format);

		if (format == AuNone) {
			test_format = SDL_NextAudioFormat();
		}
	}
	if ( format == 0 ) {
		SDL_SetError("Couldn't find any hardware audio formats");
		return(-1);
	}
	spec->format = test_format;

	this->hidden->aud = AuOpenServer("", 0, NULL, 0, NULL, NULL);
	if (this->hidden->aud == 0)
	{
		SDL_SetError("Couldn't open connection to NAS server");
		return (-1);
	}
	
	this->hidden->dev = find_device(this, spec->channels);
	if ((this->hidden->dev == AuNone) || (!(this->hidden->flow = AuCreateFlow(this->hidden->aud, NULL)))) {
		AuCloseServer(this->hidden->aud);
		this->hidden->aud = 0;
		SDL_SetError("Couldn't find a fitting playback device on NAS server");
		return (-1);
	}
	
	buffer_size = spec->freq;
	if (buffer_size < 4096)
		buffer_size = 4096; 

	if (buffer_size > 32768)
		buffer_size = 32768; /* So that the buffer won't get unmanageably big. */

	/* Calculate the final parameters for this audio specification */
	SDL_CalculateAudioSpec(spec);

	this2 = this->hidden;

	AuMakeElementImportClient(elms, spec->freq, format, spec->channels, AuTrue,
				buffer_size, buffer_size / 4, 0, NULL);
	AuMakeElementExportDevice(elms+1, 0, this->hidden->dev, spec->freq,
				AuUnlimitedSamples, 0, NULL);
	AuSetElements(this->hidden->aud, this->hidden->flow, AuTrue, 2, elms, NULL);
	AuRegisterEventHandler(this->hidden->aud, AuEventHandlerIDMask, 0, this->hidden->flow,
				event_handler, (AuPointer) NULL);

	AuStartFlow(this->hidden->aud, this->hidden->flow, NULL);

	/* Allocate mixing buffer */
	this->hidden->mixlen = spec->size;
	this->hidden->mixbuf = (Uint8 *)SDL_AllocAudioMem(this->hidden->mixlen);
	if ( this->hidden->mixbuf == NULL ) {
		return(-1);
	}
	SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

	/* Get the parent process id (we're the parent of the audio thread) */
	this->hidden->parent = getpid();

	/* We're ready to rock and roll. :-) */
	return(0);
}
