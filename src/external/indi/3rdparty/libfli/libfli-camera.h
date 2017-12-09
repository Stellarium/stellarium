/*

  Copyright (c) 2002 Finger Lakes Instrumentation (FLI), L.L.C.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

        Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

        Redistributions in binary form must reproduce the above
        copyright notice, this list of conditions and the following
        disclaimer in the documentation and/or other materials
        provided with the distribution.

        Neither the name of Finger Lakes Instrumentation (FLI), LLC
        nor the names of its contributors may be used to endorse or
        promote products derived from this software without specific
        prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

  ======================================================================

  Finger Lakes Instrumentation, L.L.C. (FLI)
  web: http://www.fli-cam.com
  email: support@fli-cam.com

*/

#ifndef _LIBFLI_CAMERA_H_
#define _LIBFLI_CAMERA_H_

#define CAPABILITY_VIDEO					(0x00010000)
#define CAPABILITY_TDI						(0x00000001)
#define CAPABILITY_BGFLUSH				(0x00000002)

#define SUPPORTS_VIDEO(x) ((((flicamdata_t *) (x->device_data))->capabilities & CAPABILITY_VIDEO) != 0)
#define SUPPORTS_TDI(x) ((((flicamdata_t *) (x->device_data))->capabilities & CAPABILITY_TDI) != 0)
#define SUPPORTS_BGFLUSH(x) ((((flicamdata_t *) (x->device_data))->capabilities & CAPABILITY_BGFLUSH) != 0)
#define SUPPORTS_END_EXPOSURE(x) ((x->devinfo.fwrev >= 0x0120) && (x->devinfo.devid == FLIUSB_PROLINE_ID) != 0)
#define SUPPORTS_SOFTWARE_TRIGGER(x) ((x->devinfo.fwrev >= 0x0120) && (x->devinfo.devid == FLIUSB_PROLINE_ID) != 0)

/* Video mode stuff */
typedef enum {
	VIDEO_MODE_OFF = 0,
	VIDEO_MODE_BEGIN,
	VIDEO_MODE_ON
} video_mode_t;

typedef struct {
  int x;                        /* X coordinate */
  int y;                        /* Y coordinate */
} point_t;

typedef struct {
  point_t ul;                  /* Upper-left */
  point_t lr;                  /* Lower-right */
} area_t;

/* CCD Parameter list */
typedef struct
{
  short index;
  char *model;
  area_t array_area;
  area_t visible_area;
  double fillfactor;
  double pixelwidth;
  double pixelheight;
} fliccdinfo_t;

typedef struct {
  long readto;
  long writeto;
  long dirto;
  fliccdinfo_t ccd;

  /* Acquisistion parameters */
  area_t image_area;
  long vbin;
  long hbin;
  long vflushbin;
  long hflushbin;
  long exposure;
  long expdur;
  long expmul;
  long frametype;
  long flushes;
  long bitdepth;
  long exttrigger;
  long exttriggerpol;
	long extexposurectrl;
	long tdirate;
	long tdiflags;

  double tempslope;
  double tempintercept;

  long grabrowcount;
  long grabrowcounttot;
  long grabrowindex;
  long grabrowwidth;
  long grabrowbatchsize;
  long grabrowbufferindex;
  long flushcountbeforefirstrow;
  long flushcountafterlastrow;

	double pix_sum;
	double pix_cnt;

	/* Variables for Proline/Microline readout */
	long top_height;
	long top_offset;
	long bottom_height;
	long bottom_offset;
	long left_width;
	long left_offset;
	long right_width;
	long right_offset;
	long dl_index;
	size_t bytesleft;

	unsigned short *ibuf_wr_idx;

	/* Booleans and state variables */
	int removebias;
	int biasoffset;
	int background_flush;
	int force_overscan;
	video_mode_t video_mode;

	/* Capability flags */
	long capabilities;

  unsigned short *gbuf;
  unsigned short *ibuf;
  size_t gbuf_siz;
  size_t ibuf_siz;
  long max_usb_xfer;
  
} flicamdata_t;

extern const fliccdinfo_t knowndev[];

long fli_camera_open(flidev_t dev);
long fli_camera_close(flidev_t dev);
long fli_camera_command(flidev_t dev, int cmd, int argc, ...);

#endif /* _LIBFLI_CAMERA_H_ */
