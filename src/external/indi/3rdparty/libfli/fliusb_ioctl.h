/*

  Copyright (c) 2005 Finger Lakes Instrumentation (FLI), L.L.C.
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

        Neither the name of Finger Lakes Instrumentation (FLI), L.L.C.
        nor the names of its contributors may be used to endorse or
        promote products derived from this software without specific
        prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
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

#ifndef _FLIUSB_IOCTL_H_
#define _FLIUSB_IOCTL_H_

#include <linux/ioctl.h>

/* Structure to describe string descriptor transfers */
typedef struct {
  unsigned int index;
  char buf[64];
} fliusb_string_descriptor_t;

/* Structure to describe bulk transfers */
typedef struct {
  u_int8_t ep;
  void *buf;
  size_t count;
  unsigned int timeout;		/* in msec */
} fliusb_bulktransfer_t;

/* 8-bit special value to identify ioctl 'type' */
#define FLIUSB_IOC_TYPE			0xf1

/* Recognized ioctl commands */
#define FLIUSB_GETRDEPADDR		_IOR(FLIUSB_IOC_TYPE, 1, u_int8_t)
#define FLIUSB_SETRDEPADDR		_IOW(FLIUSB_IOC_TYPE, 2, u_int8_t)

#define FLIUSB_GETWREPADDR		_IOR(FLIUSB_IOC_TYPE, 3, u_int8_t)
#define FLIUSB_SETWREPADDR		_IOW(FLIUSB_IOC_TYPE, 4, u_int8_t)

#define FLIUSB_GETBUFFERSIZE		_IOR(FLIUSB_IOC_TYPE, 5, size_t)
#define FLIUSB_SETBUFFERSIZE		_IOW(FLIUSB_IOC_TYPE, 6, size_t)

#define FLIUSB_GETTIMEOUT		_IOR(FLIUSB_IOC_TYPE, 7, unsigned int)
#define FLIUSB_SETTIMEOUT		_IOW(FLIUSB_IOC_TYPE, 8, unsigned int)

#define FLIUSB_BULKREAD			_IOW(FLIUSB_IOC_TYPE, 9, fliusb_bulktransfer_t)
#define FLIUSB_BULKWRITE		_IOW(FLIUSB_IOC_TYPE, 10, fliusb_bulktransfer_t)

#define FLIUSB_GET_DEVICE_DESCRIPTOR	_IOR(FLIUSB_IOC_TYPE, 11, struct usb_device_descriptor)
#define FLIUSB_GET_STRING_DESCRIPTOR	_IOR(FLIUSB_IOC_TYPE, 12, struct usb_device_descriptor)

#define FLIUSB_IOC_MAX			12

#endif /* _FLIUSB_IOCTL_H_ */
