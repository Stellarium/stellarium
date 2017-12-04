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

#ifndef _FLIUSB_H_
#define _FLIUSB_H_

#define FLIUSB_NAME       "fliusb"
#define FLIUSB_MINOR_BASE 240 /* This is arbitrary */

#define FLIUSB_VENDORID 0x0f18

/* Recognized FLI USB products */

/* FLIUSB_PROD(name, prodid) */
#define FLIUSB_PRODUCTS                     \
    FLIUSB_PROD(FLIUSB_MAXCAM, 0x0002)      \
    FLIUSB_PROD(FLIUSB_STEPPER, 0x0005)     \
    FLIUSB_PROD(FLIUSB_FOCUSER, 0x0006)     \
    FLIUSB_PROD(FLIUSB_FILTERWHEEL, 0x0007) \
    FLIUSB_PROD(FLIUSB_PROLINECAM, 0x000a)

enum
{

#define FLIUSB_PROD(name, prodid) name##_PRODID = prodid,

    FLIUSB_PRODUCTS

#undef FLIUSB_PROD

};

/* Default values (module parameters override these) */
#define FLIUSB_TIMEOUT 5000 /* milliseconds */
#ifndef SGREAD
#define FLIUSB_BUFFERSIZE 65536
#else
#define FLIUSB_BUFFERSIZE PAGE_SIZE
#endif /* SGREAD */

/* Model-specific parameters */
#define FLIUSB_RDEPADDR         0x82
#define FLIUSB_WREPADDR         0x02
#define FLIUSB_PROLINE_RDEPADDR 0x81
#define FLIUSB_PROLINE_WREPADDR 0x01

#ifdef SGREAD

#define NUMSGPAGE 32

typedef struct
{
    struct page *userpg[NUMSGPAGE];
    struct scatterlist slist[NUMSGPAGE];
    unsigned int maxpg;
    struct usb_sg_request sgreq;
    struct timer_list timer;
    struct semaphore sem;
} fliusbsg_t;

#endif /* SGREAD */

typedef struct
{
    /* Bulk transfer pipes used for read()/write() */
    unsigned int rdbulkpipe;
    unsigned int wrbulkpipe;

    /* Kernel buffer used for bulk reads */
    void *buffer;
    unsigned int buffersize;
    struct semaphore buffsem;

#ifdef SGREAD
    fliusbsg_t usbsg;
#endif

    unsigned int timeout; /* timeout for bulk transfers in milliseconds */

    struct usb_device *usbdev;
    struct usb_interface *interface;

    struct kref kref;
} fliusb_t;

#define FLIUSB_ERR(fmt, args...) printk(KERN_ERR "%s[%d]: " fmt "\n", __FUNCTION__, __LINE__, ##args)

#define FLIUSB_WARN(fmt, args...) printk(KERN_WARNING "%s[%d]: " fmt "\n", __FUNCTION__, __LINE__, ##args)

#define FLIUSB_INFO(fmt, args...) printk(KERN_NOTICE "%s[%d]: " fmt "\n", __FUNCTION__, __LINE__, ##args)

#ifdef DEBUG
#define FLIUSB_DBG(fmt, args...) FLIUSB_INFO(fmt, ##args)
#else
#define FLIUSB_DBG(fmt, args...) \
    do                           \
    {                            \
    } while (0)
#endif

#endif /* _FLIUSB_H_ */
