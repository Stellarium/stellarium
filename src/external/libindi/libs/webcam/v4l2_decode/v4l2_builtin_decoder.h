/*
    Copyright (C) 2005 by Jasem Mutlaq <mutlaqja@ikarustech.com>
    Copyright (C) 2014 by geehalel <geehalel@gmail.com>

    V4L2 Builtin Decoder

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

#include "v4l2_decode.h"

#include <map>

class V4L2_Builtin_Decoder : public V4L2_Decoder
{
    struct format
    {
        unsigned int fourcc; // V4L2 format
        unsigned char bpp;   // bytes per pixel implementation
        bool softcrop;       // softcropping available
        format(unsigned int f, unsigned char b = 8, bool sc = false) : fourcc(f), bpp(b), softcrop(sc) {}
    };

  public:
    V4L2_Builtin_Decoder();
    virtual ~V4L2_Builtin_Decoder();
    virtual void init();
    virtual bool setcrop(struct v4l2_crop c);
    virtual void resetcrop();
    virtual void usesoftcrop(bool c);
    virtual void setformat(struct v4l2_format f, bool use_ext_pix_format);
    virtual bool issupportedformat(unsigned int format);
    virtual const std::vector<unsigned int> &getsupportedformats();
    virtual void decode(unsigned char *frame, struct v4l2_buffer *buf);
    virtual unsigned char *getY();
    virtual unsigned char *getU();
    virtual unsigned char *getV();
    //virtual unsigned char * getColorBuffer();
    virtual unsigned char *getRGBBuffer();
    virtual float *getLinearY();
    virtual int getBpp();
    virtual void setQuantization(bool);
    virtual void setLinearization(bool);

  protected:
    void init_supported_formats();
    std::map<unsigned int, struct format *> supported_formats;
    std::vector<unsigned int> vsuppformats;
    void allocBuffers();
    void makeY();
    void makeLinearY();

    struct v4l2_crop crop;
    struct v4l2_format fmt;
    bool useSoftCrop; // uses software cropping
    bool doCrop;      // do software cropping when decoding frames
    bool doQuantization;
    bool doLinearization;

    unsigned char *YBuf;
    unsigned char *UBuf;
    unsigned char *VBuf;
    unsigned char *yuvBuffer;
    unsigned char *yuyvBuffer;
    unsigned char *colorBuffer;
    unsigned char *rgb24_buffer;
    float *linearBuffer;
    //unsigned char *cropbuf;
    unsigned int bufwidth;
    unsigned int bufheight;
    char lut5[32];
    char lut6[64];
    unsigned char bpp;
};
