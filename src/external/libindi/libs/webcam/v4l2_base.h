/*
    Copyright (C) 2005 by Jasem Mutlaq <mutlaqja@ikarustech.com>
    Copyright (C) 2013 Geehalel (geehalel@gmail.com)

    Based on V4L 2 Example
    http://v4l2spec.bytesex.org/spec-single/v4l2.html#CAPTURE-EXAMPLE

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#pragma once

//#include "videodev2.h"
#include "eventloop.h"
#include "indidevapi.h"
// this adds add dependency to indidriver for v4l_legacy, meade_lpi

// Can't use logger as legacy drivers don't use defaultdevice
//#include "indilogger.h"
#include "v4l2_decode/v4l2_decode.h"
// for direct recording
#include "stream/streammanager.h"

#include <stdio.h>
#include <cstdlib>

#include <linux/videodev2.h>

#define VIDEO_COMPRESSION_LEVEL 4

class V4L2_Driver;

enum
{
    LX_ACTIVE = 0,
    LX_TRIGGERED,
    LX_ACCUMULATING
};

namespace INDI
{

class V4L2_Base
{
  public:
    V4L2_Base();
    virtual ~V4L2_Base();

    typedef enum { IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR } io_method;

    struct buffer
    {
        void *start;
        size_t length;
    };

    /* Connection */
    virtual int connectCam(const char *devpath, char *errmsg, int pixelFormat = -1, int width = -1, int height = -1);
    virtual void disconnectCam(bool stopcapture);
    char *getDeviceName();
    void setDeviceName(const char *name);
    bool isLXmodCapable();

    /* Updates */
    void callFrame(void *p);

    /* Image Format/Size */
    int getFormat();
    int getWidth();
    int getHeight();
    int getBpp();
    virtual int setSize(int x, int y);
    virtual void getMaxMinSize(int &x_max, int &y_max, int &x_min, int &y_min);

    /* Frame rate */
    int (V4L2_Base::*setframerate)(struct v4l2_fract frate, char *errmsg);
    struct v4l2_fract (V4L2_Base::*getframerate)();

    unsigned char *getY();
    unsigned char *getU();
    unsigned char *getV();
    // 2017-01-24 JM: Deprecated RGBA (32bit) buffer. Should use RGB24 buffer to save space
    //unsigned char * getColorBuffer();
    unsigned char *getRGBBuffer();
    float *getLinearY();

    void registerCallback(WPF *fp, void *ud);

    int start_capturing(char *errmsg);
    int stop_capturing(char *errmsg);
    static void newFrame(int fd, void *p);

    //void setDropFrameCount(unsigned int count) { dropFrameCount = count;}
    void enumerate_ctrl();
    void enumerate_menu();
    bool enumerate_ext_ctrl();
    int queryINTControls(INumberVectorProperty *nvp);
    bool queryExtControls(INumberVectorProperty *nvp, unsigned int *nnumber, ISwitchVectorProperty **options,
                          unsigned int *noptions, const char *dev, const char *group);
    void queryControls(INumberVectorProperty *nvp, unsigned int *nnumber, ISwitchVectorProperty **options,
                       unsigned int *noptions, const char *dev, const char *group);

    int getControl(unsigned int ctrl_id, double *value, char *errmsg);
    int setINTControl(unsigned int ctrl_id, double new_value, char *errmsg);
    int setOPTControl(unsigned int ctrl_id, unsigned int new_value, char *errmsg);

    int query_ctrl(unsigned int ctrl_id, double &ctrl_min, double &ctrl_max, double &ctrl_step, double &ctrl_value,
                   char *errmsg);
    void getinputs(ISwitchVectorProperty *inputssp);
    int setinput(unsigned int inputindex, char *errmsg);
    void getcaptureformats(ISwitchVectorProperty *captureformatssp);
    int setcaptureformat(unsigned int captureformatindex, char *errmsg);
    void getcapturesizes(ISwitchVectorProperty *capturesizessp, INumberVectorProperty *capturesizenp);
    int setcapturesize(unsigned int w, unsigned int h, char *errmsg);
    void getframerates(ISwitchVectorProperty *frameratessp, INumberVectorProperty *frameratenp);
    int setcroprect(int x, int y, int w, int h, char *errmsg);
    struct v4l2_rect getcroprect();

    void setColorProcessing(bool quantization, bool colorconvert, bool linearization);

    void setlxstate(short s)
    {
        IDLog("setlexstate to %d\n", s);
        lxstate = s;
    }
    short getlxstate() { return lxstate; }
    bool isstreamactive() { return streamactive; }

    void doDecode(bool);    

  protected:
    int xioctl(int fd, int request, void *arg, char const *const request_str);
    int ioctl_set_format(struct v4l2_format new_fmt, char *errmsg);

    int read_frame(char *errsg);
    int uninit_device(char *errmsg);
    int open_device(const char *devpath, char *errmsg);
    int check_device(char *errmsg);
    int init_device(char *errmsg);
    int init_mmap(char *errmsg);
    int errno_exit(const char *s, char *errmsg);

    void close_device();
    void init_userp(unsigned int buffer_size);
    void init_read(unsigned int buffer_size);

    void findMinMax();

    int enumeratedInputs;
    int enumeratedCaptureFormats;

    /* Frame rate */
    int stdsetframerate(struct v4l2_fract frate, char *errmsg);
    int pwcsetframerate(struct v4l2_fract frate, char *errmsg);
    struct v4l2_fract stdgetframerate();

    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_input input;
    struct v4l2_buffer buf;

    bool cancrop;
    bool cropset;
    bool cansetrate;
    bool streamedonce;
    bool streamactive;

    short lxstate;

    struct v4l2_queryctrl queryctrl;
    struct v4l2_querymenu querymenu;
    bool has_ext_pix_format;

    bool is_compressed() const;

    WPF *callback;
    void *uptr;
    char dev_name[64];
    const char *path;
    io_method io;
    int fd;
    struct buffer *buffers;
    unsigned int n_buffers;
    bool reallocate_buffers;
    //int		dropFrame;
    //bool      dropFrameEnabled;
    //unsigned int      dropFrameCount;

    struct v4l2_fract frameRate;
    int xmax, xmin, ymax, ymin;
    int selectCallBackID;
    //unsigned char * YBuf,*UBuf,*VBuf, *yuvBuffer, *colorBuffer, *rgb24_buffer, *cropbuf;

    V4L2_Decode *v4l2_decode;
    V4L2_Decoder *decoder;
    bool dodecode;

    int bpp;

    friend class ::V4L2_Driver;

    char deviceName[MAXINDIDEVICE];
};
}
