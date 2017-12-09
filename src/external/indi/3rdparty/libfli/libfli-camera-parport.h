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

#ifndef _LIBFLI_CAMERA_PARPORT_H_
#define _LIBFLI_CAMERA_PARPORT_H_

/* Define command and data word formats */
#define C_ADDRESS(addr,ext) (0x8000|(((addr)<<8)&0x0f00)|((ext)&0x00ff))
#define C_RESTCFG(gain,chnl,exttrig,res) (0x9000|(((gain)<<8)&0x0f00)|(((chnl)<<5)&0x00e0)|(((exttrig)<<4)&0x0010)|(((res)&0x000f)))
#define C_SHUTTER(open,dmult) (0xa000|((dmult)&0x07ff)|(((open)<<11)&0x0800))
#define C_SEND(x) (0xb000|((x)&0x0fff))
#define C_FLUSH(x) (0xc000|((x)&0x0fff))
#define C_VSKIP(x) (0xd000|((x)&0x0fff))
#define C_HSKIP(x) (0xe000|((x)&0x0fff))
#define C_TEMP(x) (0xf000|((x)&0x0fff))
#define D_XROWOFF(x) (0x0000|((x)&0x0fff))
#define D_XROWWID(x) (0x1000|((x)&0x0fff))
#define D_XFLBIN(x) (0x2000|((x)&0x0fff))
#define D_YFLBIN(x) (0x3000|((x)&0x0fff))
#define D_XBIN(x) (0x4000|((x)&0x0fff))
#define D_YBIN(x) (0x5000|((x)&0x0fff))
#define D_EXPDUR(x) (0x6000|((x)&0x0fff))
#define D_RESERVE(x) (0x7000|((x)&0x0fff))

/* Define extended parameter fields for querying camera */
#define EPARAM_ECHO (0x00)
#define EPARAM_CCDID (0x01)
#define EPARAM_FIRM (0x02)
#define EPARAM_SNHIGH (0x03)
#define EPARAM_SNLOW (0x04)
#define EPARAM_SIGGAIN (0x05)
#define EPARAM_DEVICE (0x06)

/* I/O Bit definitions */
#define FLICCD_IO_P0	(0x01)
#define FLICCD_IO_P1	(0x02)
#define FLICCD_IO_P2	(0x04)
#define FLICCD_IO_P3	(0x08)

long fli_camera_parport_open(flidev_t dev);
long fli_camera_parport_get_array_area(flidev_t dev, long *ul_x, long *ul_y,
				       long *lr_x, long *lr_y);
long fli_camera_parport_get_visible_area(flidev_t dev, long *ul_x, long *ul_y,
					 long *lr_x, long *lr_y);
long fli_camera_parport_set_exposure_time(flidev_t dev, long exptime);
long fli_camera_parport_set_image_area(flidev_t dev, long ul_x, long ul_y,
				       long lr_x, long lr_y);
long fli_camera_parport_set_hbin(flidev_t dev, long hbin);
long fli_camera_parport_set_vbin(flidev_t dev, long vbin);
long fli_camera_parport_get_exposure_status(flidev_t dev, long *timeleft);
long fli_camera_parport_set_temperature(flidev_t dev, double temperature);
long fli_camera_parport_get_temperature(flidev_t dev, double *temperature);
long fli_camera_parport_grab_row(flidev_t dev, void *buf, size_t width);
long fli_camera_parport_expose_frame(flidev_t dev);
long fli_camera_parport_flush_rows(flidev_t dev, long rows, long repeat);
long fli_camera_parport_set_bit_depth(flidev_t dev, flibitdepth_t bitdepth);
long fli_camera_parport_read_ioport(flidev_t dev, long *ioportset);
long fli_camera_parport_write_ioport(flidev_t dev, long ioportset);
long fli_camera_parport_configure_ioport(flidev_t dev, long ioportset);
long fli_camera_parport_control_shutter(flidev_t dev, long shutter);

#endif /* _LIBFLI_CAMERA_PARPORT_H_ */
