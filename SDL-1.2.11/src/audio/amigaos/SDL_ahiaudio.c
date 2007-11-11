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
*/
#include "SDL_config.h"

/* Allow access to a raw mixing buffer (for AmigaOS) */

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_ahiaudio.h"

/* Audio driver functions */
static int AHI_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void AHI_WaitAudio(_THIS);
static void AHI_PlayAudio(_THIS);
static Uint8 *AHI_GetAudioBuf(_THIS);
static void AHI_CloseAudio(_THIS);

#ifndef __SASC
	#define mymalloc(x) AllocVec(x,MEMF_PUBLIC) 
	#define myfree FreeVec
#else
	#define mymalloc malloc
	#define myfree free
#endif

/* Audio driver bootstrap functions */

static int Audio_Available(void)
{
	int ok=0;
	struct MsgPort *p;
	struct AHIRequest *req;

	if(p=CreateMsgPort())
	{
		if(req=(struct AHIRequest *)CreateIORequest(p,sizeof(struct AHIRequest)))
		{
			req->ahir_Version=4;

			if(!OpenDevice(AHINAME,0,(struct IORequest *)req,NULL))
			{
				D(bug("AHI available.\n"));
				ok=1;
				CloseDevice((struct IORequest *)req);
			}
			DeleteIORequest((struct IORequest *)req);
		}
		DeleteMsgPort(p);
	}

	D(if(!ok) bug("AHI not available\n"));
	return ok;
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

#ifndef NO_AMIGADEBUG
	D(bug("AHI created...\n"));
#endif

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
	this->OpenAudio = AHI_OpenAudio;
	this->WaitAudio = AHI_WaitAudio;
	this->PlayAudio = AHI_PlayAudio;
	this->GetAudioBuf = AHI_GetAudioBuf;
	this->CloseAudio = AHI_CloseAudio;

	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap AHI_bootstrap = {
	"AHI", Audio_Available, Audio_CreateDevice
};


void static AHI_WaitAudio(_THIS)
{
	if(!CheckIO((struct IORequest *)audio_req[current_buffer]))
	{
		WaitIO((struct IORequest *)audio_req[current_buffer]);
//		AbortIO((struct IORequest *)audio_req[current_buffer]);
	}
}

static void AHI_PlayAudio(_THIS)
{
	if(playing>1)
		WaitIO((struct IORequest *)audio_req[current_buffer]);

	/* Write the audio data out */
	audio_req[current_buffer] -> ahir_Std. io_Message.mn_Node.ln_Pri = 60;
	audio_req[current_buffer] -> ahir_Std. io_Data                = mixbuf[current_buffer];
	audio_req[current_buffer] -> ahir_Std. io_Length              = this->hidden->size;
	audio_req[current_buffer] -> ahir_Std. io_Offset              = 0;
	audio_req[current_buffer] -> ahir_Std . io_Command            = CMD_WRITE;
	audio_req[current_buffer] -> ahir_Frequency                   = this->hidden->freq;
	audio_req[current_buffer] -> ahir_Volume                      = 0x10000;
	audio_req[current_buffer] -> ahir_Type                        = this->hidden->type;
	audio_req[current_buffer] -> ahir_Position                    = 0x8000;
	audio_req[current_buffer] -> ahir_Link                        = (playing>0 ? audio_req[current_buffer^1] : NULL);

	SendIO((struct IORequest *)audio_req[current_buffer]);
	current_buffer^=1;

	playing++;
}

static Uint8 *AHI_GetAudioBuf(_THIS)
{
	return(mixbuf[current_buffer]);
}

static void AHI_CloseAudio(_THIS)
{
	D(bug("Closing audio...\n"));

	playing=0;

	if(audio_req[0])
	{
		if(audio_req[1])
		{
			D(bug("Break req[1]...\n"));

			AbortIO((struct IORequest *)audio_req[1]);
			WaitIO((struct IORequest *)audio_req[1]);
		}

		D(bug("Break req[0]...\n"));

		AbortIO((struct IORequest *)audio_req[0]);
		WaitIO((struct IORequest *)audio_req[0]);

		if(audio_req[1])
		{
			D(bug("Break AGAIN req[1]...\n"));
			AbortIO((struct IORequest *)audio_req[1]);
			WaitIO((struct IORequest *)audio_req[1]);
		}
// Double abort to be sure to break the dbuffering process.

		SDL_Delay(200);

		D(bug("Reqs breaked, closing device...\n"));
		CloseDevice((struct IORequest *)audio_req[0]);
		D(bug("Device closed, freeing memory...\n"));
		myfree(audio_req[1]);
		D(bug("Memory freed, deleting IOReq...\n")); 
		DeleteIORequest((struct IORequest *)audio_req[0]);
		audio_req[0]=audio_req[1]=NULL;
	}

	D(bug("Freeing mixbuf[0]...\n"));
	if ( mixbuf[0] != NULL ) {
		myfree(mixbuf[0]);
//		SDL_FreeAudioMem(mixbuf[0]);
		mixbuf[0] = NULL;
	}

	D(bug("Freeing mixbuf[1]...\n"));
	if ( mixbuf[1] != NULL ) {
		myfree(mixbuf[1]);
//		SDL_FreeAudioMem(mixbuf[1]);
		mixbuf[1] = NULL;
	}

	D(bug("Freeing audio_port...\n"));

	if ( audio_port != NULL ) {
		DeleteMsgPort(audio_port);
		audio_port = NULL;
	}
	D(bug("...done!\n"));
}

static int AHI_OpenAudio(_THIS, SDL_AudioSpec *spec)
{	
//	int width;

	D(bug("AHI opening...\n"));

	/* Determine the audio parameters from the AudioSpec */
	switch ( spec->format & 0xFF ) {

		case 8: { /* Signed 8 bit audio data */
			D(bug("Samples a 8 bit...\n"));
			spec->format = AUDIO_S8;
			this->hidden->bytespersample=1;
			if(spec->channels<2)
				this->hidden->type = AHIST_M8S;
			else
				this->hidden->type = AHIST_S8S;
		}
		break;

		case 16: { /* Signed 16 bit audio data */
			D(bug("Samples a 16 bit...\n"));
			spec->format = AUDIO_S16MSB;
			this->hidden->bytespersample=2;
			if(spec->channels<2)
				this->hidden->type = AHIST_M16S;
			else
				this->hidden->type = AHIST_S16S;
		}
		break;

		default: {
			SDL_SetError("Unsupported audio format");
			return(-1);
		}
	}

	if(spec->channels!=1 && spec->channels!=2)
	{
		D(bug("Wrong channel number!\n"));
		SDL_SetError("Channel number non supported");
		return -1;
	}

	D(bug("Before CalculateAudioSpec\n"));
	/* Update the fragment size as size in bytes */
	SDL_CalculateAudioSpec(spec);

	D(bug("Before CreateMsgPort\n"));

	if(!(audio_port=CreateMsgPort()))
	{
		SDL_SetError("Unable to create a MsgPort");
		return -1;
	}

	D(bug("Before CreateIORequest\n"));

	if(!(audio_req[0]=(struct AHIRequest *)CreateIORequest(audio_port,sizeof(struct AHIRequest))))
	{
		SDL_SetError("Unable to create an AHIRequest");
		DeleteMsgPort(audio_port);
		return -1;
	}

	audio_req[0]->ahir_Version = 4;

	if(OpenDevice(AHINAME,0,(struct IORequest *)audio_req[0],NULL))
	{
		SDL_SetError("Unable to open AHI device!\n");
		DeleteIORequest((struct IORequest *)audio_req[0]);
		DeleteMsgPort(audio_port);
		return -1;
	}
	
	D(bug("AFTER opendevice\n"));

	/* Set output frequency and size */
	this->hidden->freq = spec->freq;
	this->hidden->size = spec->size;

	D(bug("Before buffer allocation\n"));

	/* Allocate mixing buffer */
	mixbuf[0] = (Uint8 *)mymalloc(spec->size);
	mixbuf[1] = (Uint8 *)mymalloc(spec->size);

	D(bug("Before audio_req allocation\n"));

	if(!(audio_req[1]=mymalloc(sizeof(struct AHIRequest))))
	{
		SDL_OutOfMemory();
		return(-1);
	}
	
	D(bug("Before audio_req memcpy\n"));

	SDL_memcpy(audio_req[1],audio_req[0],sizeof(struct AHIRequest));

	if ( mixbuf[0] == NULL || mixbuf[1] == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}

	D(bug("Before mixbuf memset\n"));

	SDL_memset(mixbuf[0], spec->silence, spec->size);
	SDL_memset(mixbuf[1], spec->silence, spec->size);

	current_buffer=0;
	playing=0;

	D(bug("AHI opened: freq:%ld mixbuf:%lx/%lx buflen:%ld bits:%ld channels:%ld\n",spec->freq,mixbuf[0],mixbuf[1],spec->size,this->hidden->bytespersample*8,spec->channels));

	/* We're ready to rock and roll. :-) */
	return(0);
}
