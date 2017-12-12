/*
    Copyright (C) 2005 by Jasem Mutlaq <mutlaqja@ikarustech.com>
    Copyright (C) 2014 by geehalel <geehalel@gmail.com>

    V4L2 Builtin Decoder

    As of August 2015 gst-lauch-1.0 and v4l2loopback do not work together well (https://github.com/umlaeute/v4l2loopback/issues/83)
    Still use v4l2loopback (see below) but  with ffmpeg:
    ffmpeg -f lavfi -re -i "smptebars=size=640x480:rate=30" -pix_fmt yuv420p -f v4l2  /dev/video8
    Use the -re flag to really get frame rate otherwise ffmpeg outputs frames as fast as possible (400 to 700fps).
    With indi-opencv-ccd use /dev/video1 as v4l2loopback device
    for 16 bits gray
    ffmpeg -f lavfi -re -i "testsrc=size=640x480:rate=30" -pix_fmt gray16le -f v4l2  /dev/video1
    Profiling
     cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_CXX_FLAGS=-pg /nfs/devel/sourceforge/indi-code/libindi/
     kill indiserver with the kill command, not ctrl-c

    To test decoders, use gstreamer and the v4l2loopback kernel module
    Use the experimental git branch of v4l2loopback to get more pixel formats
    git clone https://github.com/umlaeute/v4l2loopback/ -b experimental
    cd v4l2loopback; make; sudo make install
    sudo modprobe v4l2loopback video_nr=8 card_label="Indi V4L2 Test Loopback" # debug=n
    gst-launch-1.0  -v videotestsrc  ! video/x-raw,format=\(string\)UYVY,width=1024,height=576,framerate=\(fraction\)30/1 ! v4l2sink device=/dev/video8
    gst-launch-1.0  -v v4l2src device=/dev/video0  ! jpegdec ! videoconvert! video/x-raw,format=\(string\)UYVY,width=1280,height=960,framerate=\(fraction\)5/1 ! v4l2sink device=/dev/video8

    For Bayer I used gst-launch-0.10
    modprobe v4l2loopback video_nr=8 card_label="Indi Loopback" exclusive_caps=0,0
    gst-launch-0.10 -v videotestsrc ! 'video/x-raw-bayer, format=(string)bggr, width=640, height=480, framerate=(fraction)2/1' ! v4l2sink device=/dev/video8

    For Gray16 format I use gst-launch with a videotstsrc in GRAY16 format writing in a FIFO and a C program reding the FIFO into the v4l2loopback device
    mkfifo /tmp/videopipe
    gst-launch-1.0  -v videotestsrc  ! video/x-raw,format=\(string\)GRAY16_LE,width=1024,height=576,framerate=\(fraction\)25/1 ! filesink location =/tmp/videopipe
    ./gray16_to_v4l2 /dev/video8 < /tmp/videopipe &
    The C program ./gray16_to_v4l2 is adpated from https://github.com/umlaeute/v4l2loopback/blob/master/examples/yuv4mpeg_to_v4l2.c :
    process_header/read_header calls are suppressed (copy_frames uses a while (true) loop), frame_width and frame_height are constant
    and V4L2 pixel format is set to V4L2_PIX_FMT_Y16.

    To repeat an avi stream
    ./v4l2loopback-ctl set-caps 'video/x-raw-yuv,width=720,height=576' /dev/video8
    ./v4l2loopback-ctl set-fps 25/1 /dev/video8
    as normal user
    while true; do gst-launch-0.10 -vvv filesrc location=chi-aquarius.avi ! avidemux name=demux  demux.video_00 ! queue ! decodebin ! videoscale add-borders=true ! v4l2sink device=/dev/video8 ; done

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

#include "v4l2_builtin_decoder.h"

//#include "indilogger.h"
#include "../ccvt.h"
#include "../v4l2_colorspace.h"

#include <cstring> // memcpy

V4L2_Builtin_Decoder::V4L2_Builtin_Decoder()
{
    unsigned int i;
    name           = "Builtin decoder";
    useSoftCrop    = false;
    doCrop         = false;
    doQuantization = false;
    YBuf           = nullptr;
    UBuf           = nullptr;
    VBuf           = nullptr;
    yuvBuffer      = nullptr;
    yuyvBuffer     = nullptr;
    colorBuffer    = nullptr;
    rgb24_buffer   = nullptr;
    linearBuffer   = nullptr;
    //cropbuf = nullptr;
    for (i = 0; i < 32; i++)
    {
        lut5[i] = (char)(((float)i * 255.0) / 31.0);
    }
    for (i = 0; i < 64; i++)
    {
        lut6[i] = (char)(((float)i * 255.0) / 63.0);
    }
    initColorSpace();
    bpp = 8;
}

V4L2_Builtin_Decoder::~V4L2_Builtin_Decoder()
{
    YBuf = nullptr;
    UBuf = nullptr;
    VBuf = nullptr;
    if (yuvBuffer)
        delete[](yuvBuffer);
    yuvBuffer = nullptr;
    if (yuyvBuffer)
        delete[](yuyvBuffer);
    yuyvBuffer = nullptr;
    if (colorBuffer)
        delete[](colorBuffer);
    colorBuffer = nullptr;
    if (rgb24_buffer)
        delete[](rgb24_buffer);
    rgb24_buffer = nullptr;
    if (linearBuffer)
        delete[](linearBuffer);
    linearBuffer = nullptr;
};

void V4L2_Builtin_Decoder::init()
{
    init_supported_formats();
};

void V4L2_Builtin_Decoder::decode(unsigned char *frame, struct v4l2_buffer *buf)
{
    //DEBUG(INDI::Logger::DBG_SESSION,"Calling builtin decoder decode");
    //IDLog("Decoding buffer at %lx, len %d, bytesused %d, bytesperpixel %d, sequence %d, flag %x, field %x, use soft crop %c, do crop %c\n", frame, buf->length, buf->bytesused, fmt.fmt.pix.bytesperline, buf->sequence, buf->flags, buf->field, (useSoftCrop?'y':'n'), (doCrop?'y':'n'));

    switch (fmt.fmt.pix.pixelformat)
    {
        case V4L2_PIX_FMT_GREY:
            if (useSoftCrop && doCrop)
            {
                unsigned char *src  = frame + crop.c.left + (crop.c.top * fmt.fmt.pix.width);
                unsigned char *dest = YBuf;

                for (unsigned int i = 0; i < (unsigned int)crop.c.height; i++)
                {
                    memcpy(dest, src, crop.c.width);
                    src += fmt.fmt.pix.width;
                    dest += crop.c.width;
                }
            }
            else
            {
                memcpy(YBuf, frame, bufwidth * bufheight);
            }
            break;

        case V4L2_PIX_FMT_Y16:
            if (useSoftCrop && doCrop)
            {
                unsigned char *src  = frame + 2 * (crop.c.left) + (crop.c.top * fmt.fmt.pix.bytesperline);
                unsigned char *dest = yuyvBuffer;

                for (unsigned int i = 0; i < (unsigned int)crop.c.height; i++)
                {
                    memcpy(dest, src, 2 * crop.c.width);
                    src += fmt.fmt.pix.bytesperline;
                    dest += 2 * crop.c.width;
                }
            }
            else
            {
                memcpy(yuyvBuffer, frame, 2 * bufwidth * bufheight);
            }
            break;

        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
            if (useSoftCrop && doCrop)
            {
                unsigned char *src  = frame + crop.c.left + (crop.c.top * fmt.fmt.pix.width);
                unsigned char *dest = YBuf;

                //IDLog("grabImage: src=%d dest=%d\n", src, dest);
                for (unsigned int i = 0; i < (unsigned int)crop.c.height; i++)
                {
                    memcpy(dest, src, crop.c.width);
                    src += fmt.fmt.pix.width;
                    dest += crop.c.width;
                }

                dest = UBuf;
                src  = frame + (fmt.fmt.pix.width * fmt.fmt.pix.height) +
                      ((crop.c.left + (crop.c.top * fmt.fmt.pix.width) / 2) / 2);
                if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YVU420)
                {
                    dest = VBuf;
                }
                for (unsigned int i = 0; i < (unsigned int)crop.c.height / 2; i++)
                {
                    memcpy(dest, src, crop.c.width / 2);
                    src += fmt.fmt.pix.width / 2;
                    dest += crop.c.width / 2;
                }

                dest = VBuf;
                src  = frame + (fmt.fmt.pix.width * fmt.fmt.pix.height) +
                      ((fmt.fmt.pix.width * fmt.fmt.pix.height) / 4) +
                      ((crop.c.left + (crop.c.top * fmt.fmt.pix.width) / 2) / 2);
                if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YVU420)
                {
                    dest = UBuf;
                }
                for (unsigned int i = 0; i < (unsigned int)crop.c.height / 2; i++)
                {
                    memcpy(dest, src, crop.c.width / 2);
                    src += fmt.fmt.pix.width / 2;
                    dest += crop.c.width / 2;
                }
            }
            else
            {
                memcpy(YBuf, frame, bufwidth * bufheight);
                if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420)
                {
                    memcpy(UBuf, frame + bufwidth * bufheight, (bufwidth / 2) * (bufheight / 2));
                    memcpy(VBuf, frame + bufwidth * bufheight + (bufwidth / 2) * (bufheight / 2),
                           (bufwidth / 2) * (bufheight / 2));
                }
                else
                {
                    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YVU420)
                    {
                        memcpy(VBuf, frame + bufwidth * bufheight, (bufwidth / 2) * (bufheight / 2));
                        memcpy(UBuf, frame + bufwidth * bufheight + (bufwidth / 2) * (bufheight / 2),
                               (bufwidth / 2) * (bufheight / 2));
                    }
                }
            }
            break;

        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
            if (useSoftCrop && doCrop)
            {
                unsigned char *src  = frame + crop.c.left + (crop.c.top * fmt.fmt.pix.bytesperline);
                unsigned char *dest = YBuf, *destv = nullptr;

                //IDLog("grabImage: src=%d dest=%d\n", src, dest);
                for (unsigned int i = 0; i < (unsigned int)crop.c.height; i++)
                {
                    memcpy(dest, src, crop.c.width);
                    src += fmt.fmt.pix.bytesperline;
                    dest += crop.c.width;
                }

                dest  = UBuf;
                destv = VBuf;
                src   = frame + (fmt.fmt.pix.bytesperline * fmt.fmt.pix.height) +
                      ((crop.c.left + (crop.c.top * fmt.fmt.pix.bytesperline) / 2) / 2);
                if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV21)
                {
                    dest  = VBuf;
                    destv = UBuf;
                }
                for (unsigned int i = 0; i < (unsigned int)crop.c.height / 2; i++)
                {
                    unsigned char *s = src;
                    for (unsigned int j = 0; j < (unsigned int)crop.c.width; j += 2)
                    {
                        *(dest++)  = *(s++);
                        *(destv++) = *(s++);
                    }
                    src += fmt.fmt.pix.bytesperline;
                }
            }
            else
            {
                unsigned char *src   = frame;
                unsigned char *dest  = YBuf;
                unsigned char *destv = VBuf;
                unsigned char *s = nullptr;

                for (unsigned int i = 0; i < bufheight; i++)
                {
                    memcpy(dest, src, bufwidth);
                    src += fmt.fmt.pix.bytesperline;
                    dest += bufwidth;
                }
                dest = UBuf;
                src  = frame + (fmt.fmt.pix.bytesperline * bufheight);
                if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV21)
                {
                    dest  = VBuf;
                    destv = UBuf;
                }
                for (unsigned int i = 0; i < bufheight / 2; i++)
                {
                    s = src;
                    //IDLog("NV12: converting UV line %d at %p, offset %d, pUbuf %p offset %d,  pVbuf %p offset %d\n", i, s, s- (frame + (fmt.fmt.pix.bytesperline * bufheight)), dest, dest-UBuf, destv, destv-VBuf);
                    for (unsigned int j = 0; j < bufwidth; j += 2)
                    {
                        *(dest++)  = *(s++);
                        *(destv++) = *(s++);
                    }
                    src += fmt.fmt.pix.bytesperline;
                }
            }
            break;

        case V4L2_PIX_FMT_YUYV:
            if (useSoftCrop && doCrop)
            {
                unsigned char *src  = frame + 2 * (crop.c.left) + (crop.c.top * fmt.fmt.pix.bytesperline);
                unsigned char *dest = yuyvBuffer;

                for (unsigned int i = 0; i < (unsigned int)crop.c.height; i++)
                {
                    memcpy(dest, src, 2 * crop.c.width);
                    src += fmt.fmt.pix.bytesperline;
                    dest += 2 * crop.c.width;
                }
            }
            else
            {
                memcpy(yuyvBuffer, frame, 2 * bufwidth * bufheight);
            }
            break;

        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_VYUY:
        case V4L2_PIX_FMT_YVYU:
        {
            unsigned char *src = nullptr;
            unsigned char *dest = yuyvBuffer;
            unsigned char *s = nullptr, *s1 = nullptr, *s2 = nullptr, *s3 = nullptr, *s4 = nullptr;

            if (useSoftCrop && doCrop)
            {
                src = frame + 2 * (crop.c.left) + (crop.c.top * fmt.fmt.pix.bytesperline);
                //IDLog("Decoding UYVY with cropping %dx%d frame at %lx\n", width, height, src);
            }
            else
            {
                src = frame;
                //IDLog("Decoding UYVY  %dx%d frame at %lx\n", width, height, src);
            }
            for (int i = 0; i < (int)bufheight; i++)
            {
                s = src;
                switch (fmt.fmt.pix.pixelformat)
                {
                    case V4L2_PIX_FMT_UYVY:
                        s1 = (s + 1);
                        s2 = (s);
                        s3 = (s + 3);
                        s4 = (s + 2);
                        break;
                    case V4L2_PIX_FMT_VYUY:
                        s1 = (s + 1);
                        s2 = (s + 2);
                        s3 = (s + 3);
                        s4 = (s);
                        break;
                    case V4L2_PIX_FMT_YVYU:
                        s1 = (s);
                        s2 = (s + 3);
                        s3 = (s + 2);
                        s4 = (s + 1);
                        break;
                }
                for (int j = 0; j < (int)(bufwidth / 2); j++)
                {
                    *(dest++) = *(s1);
                    *(dest++) = *(s2);
                    *(dest++) = *(s3);
                    *(dest++) = *(s4);
                    s1 += 4;
                    s2 += 4;
                    s3 += 4;
                    s4 += 4;
                }
                src += fmt.fmt.pix.bytesperline;
            }
        }
        break;

        case V4L2_PIX_FMT_RGB24:
        {
            unsigned char *src = nullptr, *dest = rgb24_buffer;

            if (useSoftCrop && doCrop)
            {
                src = frame + (3 * (crop.c.left)) + (crop.c.top * fmt.fmt.pix.bytesperline);
            }
            else
            {
                src = frame;
            }
            for (unsigned int i = 0; i < bufheight; i++)
            {
                memcpy(dest, src, 3 * bufwidth);
                src += fmt.fmt.pix.bytesperline;
                dest += 3 * bufwidth;
            }
            //memcpy(rgb24_buffer,  frame, fmt.fmt.pix.width * fmt.fmt.pix.height * 3);
        }
        break;

        case V4L2_PIX_FMT_RGB555:
        {
            unsigned char *src, *dest = rgb24_buffer;

            if (useSoftCrop && doCrop)
            {
                src = frame + (2 * (crop.c.left)) + (crop.c.top * fmt.fmt.pix.bytesperline);
            }
            else
            {
                src = frame;
            }
            for (unsigned int i = 0; i < bufheight; i++)
            {
                unsigned char *s = src;
                for (unsigned int j = 0; j < bufwidth; j++)
                {
                    *(dest++) = lut5[((*(s + 1) & 0x7C) >> 2)];                        // R
                    *(dest++) = lut5[(((*(s + 1) & 0x03) << 3) | ((*(s)&0xE0) >> 5))]; // G
                    *(dest++) = lut5[((*s) & 0x1F)];                                   // B
                    s += 2;
                }
                src += fmt.fmt.pix.bytesperline;
            }
        }
        break;

        case V4L2_PIX_FMT_RGB565:
        {
            unsigned char *src = nullptr, *dest = rgb24_buffer;

            if (useSoftCrop && doCrop)
            {
                src = frame + (2 * (crop.c.left)) + (crop.c.top * fmt.fmt.pix.bytesperline);
            }
            else
            {
                src = frame;
            }
            for (unsigned int i = 0; i < bufheight; i++)
            {
                unsigned char *s = src;
                for (unsigned int j = 0; j < bufwidth; j++)
                {
                    *(dest++) = lut5[((*(s + 1) & 0xF8) >> 3)];                        // R
                    *(dest++) = lut6[(((*(s + 1) & 0x07) << 3) | ((*(s)&0xE0) >> 5))]; // G
                    *(dest++) = lut5[((*s) & 0x1F)];                                   // B
                    s += 2;
                }
                src += fmt.fmt.pix.bytesperline;
            }
        }
        break;

        case V4L2_PIX_FMT_SBGGR8:
            bayer2rgb24(rgb24_buffer, frame, fmt.fmt.pix.width, fmt.fmt.pix.height);
            break;

        case V4L2_PIX_FMT_SRGGB8:
            bayer_rggb_2rgb24(rgb24_buffer, frame, fmt.fmt.pix.width, fmt.fmt.pix.height);
            break;

        case V4L2_PIX_FMT_SBGGR16:
            bayer16_2_rgb24((unsigned short *)rgb24_buffer, (unsigned short *)frame, fmt.fmt.pix.width,
                            fmt.fmt.pix.height);
            break;

        case V4L2_PIX_FMT_JPEG:
        case V4L2_PIX_FMT_MJPEG:
            //mjpegtoyuv420p(yuvBuffer, ((unsigned char *) buffers[buf.index].start), fmt.fmt.pix.width, fmt.fmt.pix.height, buffers[buf.index].length);
            mjpegtoyuv420p(yuvBuffer, frame, fmt.fmt.pix.width, fmt.fmt.pix.height, buf->bytesused);
            break;
        default:
        {
            unsigned char *src = YBuf;

            for (unsigned int i = 0; i < bufheight * bufwidth; i++)
            {
                *(src++) = random() % 255;
            }
        }
    }
}

bool V4L2_Builtin_Decoder::setcrop(struct v4l2_crop c)
{
    crop = c;
    IDLog("Decoder  set crop: %dx%d at (%d, %d)\n", crop.c.width, crop.c.height, crop.c.left, crop.c.top);
    if (supported_formats[fmt.fmt.pix.pixelformat]->softcrop)
    {
        doCrop = true;
        allocBuffers();
        return true;
    }
    else
    {
        doCrop = false;
        return false;
    }
}

void V4L2_Builtin_Decoder::resetcrop()
{
    IDLog("Decoder  reset crop\n");
    doCrop = false;
    allocBuffers();
}

void V4L2_Builtin_Decoder::usesoftcrop(bool c)
{
    IDLog("Decoder usesoftcrop %s\n", (c ? "true" : "false"));
    useSoftCrop = c;
}

void V4L2_Builtin_Decoder::setformat(struct v4l2_format f, bool use_ext_pix_format)
{
    (void)use_ext_pix_format;
    fmt = f;
    if (supported_formats.count(fmt.fmt.pix.pixelformat) == 1)
        bpp = supported_formats.at(fmt.fmt.pix.pixelformat)->bpp;
    else
        bpp = 8;
    IDLog("Decoder  set format: %c%c%c%c size %dx%d bpp %d\n", (fmt.fmt.pix.pixelformat) & 0xFF,
          (fmt.fmt.pix.pixelformat >> 8) & 0xFF, (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
          (fmt.fmt.pix.pixelformat >> 24) & 0xFF, f.fmt.pix.width, f.fmt.pix.height, bpp);
    /* kernel 3.19
    if (use_ext_pix_format && fmt.fmt.pix.priv == V4L2_PIX_FMT_PRIV_MAGIC)
      IDLog("Decoder: Colorspace is %d, YCbCr encoding is %d, Quantization is %d\n", fmt.fmt.pix.colorspace, fmt.fmt.pix.ycbcr_enc, fmt.fmt.pix.quantization);
    else
    */
    IDLog("Decoder: Colorspace is %d, using default ycbcr encoding and quantization\n", fmt.fmt.pix.colorspace);
    doCrop = false;
    allocBuffers();
}

void V4L2_Builtin_Decoder::setQuantization(bool doquantization)
{
    doQuantization = doquantization;
}
void V4L2_Builtin_Decoder::setLinearization(bool dolinearization)
{
    doLinearization = dolinearization;
    if (doLinearization)
        bpp = 16;
    else if (supported_formats.count(fmt.fmt.pix.pixelformat) == 1)
        bpp = supported_formats.at(fmt.fmt.pix.pixelformat)->bpp;
    else
        bpp = 8;
}
void V4L2_Builtin_Decoder::allocBuffers()
{
    YBuf = nullptr;
    UBuf = nullptr;
    VBuf = nullptr;
    if (yuvBuffer)
        delete[](yuvBuffer);
    yuvBuffer = nullptr;
    if (yuyvBuffer)
        delete[](yuyvBuffer);
    yuyvBuffer = nullptr;
    if (colorBuffer)
        delete[](colorBuffer);
    colorBuffer = nullptr;
    if (rgb24_buffer)
        delete[](rgb24_buffer);
    rgb24_buffer = nullptr;
    if (linearBuffer)
        delete[](linearBuffer);
    linearBuffer = nullptr;
    //if (cropbuf) free(cropbuf); cropbuf=nullptr;

    if (doCrop)
    {
        bufwidth  = crop.c.width;
        bufheight = crop.c.height;
    }
    else
    {
        bufwidth  = fmt.fmt.pix.width;
        bufheight = fmt.fmt.pix.height;
    }

    switch (fmt.fmt.pix.pixelformat)
    {
        case V4L2_PIX_FMT_GREY:
        case V4L2_PIX_FMT_JPEG:
        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
            yuvBuffer = new unsigned char[(bufwidth * bufheight) + ((bufwidth * bufheight) / 2)];
            YBuf      = yuvBuffer;
            UBuf      = yuvBuffer + (bufwidth * bufheight);
            VBuf      = UBuf + ((bufwidth * bufheight) / 4);
            // bzero(Ubuf, ((bufwidth * bufheight) / 2));
            break;
        case V4L2_PIX_FMT_Y16:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_VYUY:
        case V4L2_PIX_FMT_YVYU:
            yuyvBuffer = new unsigned char[(bufwidth * bufheight) * 2];
            break;
        case V4L2_PIX_FMT_RGB24:
        case V4L2_PIX_FMT_RGB555:
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_SBGGR8:
        case V4L2_PIX_FMT_SRGGB8:
        case V4L2_PIX_FMT_SBGGR16:
            rgb24_buffer = new unsigned char[(bufwidth * bufheight) * (bpp / 8) * 3];
            break;
        default:
            yuvBuffer = new unsigned char[(bufwidth * bufheight) + ((bufwidth * bufheight) / 2)];
            YBuf      = yuvBuffer;
            UBuf      = yuvBuffer + (bufwidth * bufheight);
            VBuf      = UBuf + ((bufwidth * bufheight) / 4);
            break;
    }
    IDLog("Decoder allocBuffers cropping %s\n", (doCrop ? "true" : "false"));
}

void V4L2_Builtin_Decoder::makeLinearY()
{
    unsigned char *src = YBuf;
    float *dest;
    unsigned int i;
    if (!linearBuffer)
    {
        linearBuffer = new float[(bufwidth * bufheight)];
    }
    dest = linearBuffer;
    for (i = 0; i < bufwidth * bufheight; i++)
        *dest++ = (*src++) / 255.0;
    linearize(linearBuffer, bufwidth * bufheight, &fmt);
}
void V4L2_Builtin_Decoder::makeY()
{
    if (!yuvBuffer)
    {
        yuvBuffer = new unsigned char[(bufwidth * bufheight) + ((bufwidth * bufheight) / 2)];
        YBuf      = yuvBuffer;
        UBuf      = yuvBuffer + (bufwidth * bufheight);
        VBuf      = UBuf + ((bufwidth * bufheight) / 4);
    }
    switch (fmt.fmt.pix.pixelformat)
    {
        case V4L2_PIX_FMT_RGB24:
        case V4L2_PIX_FMT_RGB555:
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_SBGGR8:
        case V4L2_PIX_FMT_SRGGB8:
            RGB2YUV(bufwidth, bufheight, rgb24_buffer, YBuf, UBuf, VBuf, 0);
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_VYUY:
        case V4L2_PIX_FMT_YVYU:
            // todo handcopy only Ybuf using an int, byfwidth should be even
            ccvt_yuyv_420p(bufwidth, bufheight, yuyvBuffer, YBuf, UBuf, VBuf);
            break;
    }
}

unsigned char *V4L2_Builtin_Decoder::getY()
{
    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_Y16)
        return yuyvBuffer;
    makeY();
    if (doQuantization && getQuantization(&fmt) == QUANTIZATION_LIM_RANGE)
        rangeY8(YBuf, (bufwidth * bufheight));
    if (doLinearization)
    {
        unsigned int i;
        float *src;
        unsigned short *dest;
        if (!yuyvBuffer)
            yuyvBuffer = new unsigned char[(bufwidth * bufheight) * 2];
        makeLinearY();
        src  = linearBuffer;
        dest = (unsigned short *)yuyvBuffer;
        for (i = 0; i < bufwidth * bufheight; i++)
            *dest++ = (unsigned short)(*src++ * 65535.0);
        return yuyvBuffer;
    }
    return YBuf;
}

float *V4L2_Builtin_Decoder::getLinearY()
{
    makeY();
    if (doQuantization && getQuantization(&fmt) == QUANTIZATION_LIM_RANGE)
        rangeY8(YBuf, (bufwidth * bufheight));
    makeLinearY();
    return linearBuffer;
}

unsigned char *V4L2_Builtin_Decoder::getU()
{
    return UBuf;
}

unsigned char *V4L2_Builtin_Decoder::getV()
{
    return VBuf;
}

/* used for streaming/exposure */
#if 0
unsigned char * V4L2_Builtin_Decoder::geColorBuffer()
{
    //cerr << "in get color buffer " << endl;
    //IDLog("Decoder geRGBBuffer %s\n", (doCrop?"true":"false"));
    if (!colorBuffer) colorBuffer = new unsigned char[(bufwidth * bufheight) * (bpp / 8) * 4];
    switch (fmt.fmt.pix.pixelformat)
    {
        case V4L2_PIX_FMT_GREY:
        case V4L2_PIX_FMT_JPEG:
        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
            ccvt_420p_bgr32(bufwidth, bufheight, (void *)yuvBuffer, (void *)colorBuffer);
            break;

        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_VYUY:
        case V4L2_PIX_FMT_YVYU:
            ccvt_yuyv_bgr32(bufwidth, bufheight, yuyvBuffer, (void *)colorBuffer);
            break;
        case V4L2_PIX_FMT_RGB24:
        case V4L2_PIX_FMT_RGB555:
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_SBGGR8:
        case V4L2_PIX_FMT_SRGGB8:
            ccvt_rgb24_bgr32(bufwidth, bufheight, rgb24_buffer, (void *)colorBuffer);
            break;
        case V4L2_PIX_FMT_Y16:
            /* OOOps this is planar ARGB */
            /*
            bzero(colorBuffer, bufwidth * bufheight * 2);
            memcpy(colorBuffer + (bufwidth * bufheight * 2), yuyvBuffer, bufwidth * bufheight * 2);
            memcpy(colorBuffer + 2 * (bufwidth * bufheight * 2), yuyvBuffer, bufwidth * bufheight * 2);
            memcpy(colorBuffer + 3 * (bufwidth * bufheight * 2), yuyvBuffer, bufwidth * bufheight * 2);
            */
        {
            /* this is bgra , use unsigned short here... */
            unsigned int i;
            unsigned char * src = yuyvBuffer;
            unsigned char * dest = colorBuffer;
            for (i = 0; i < bufwidth * bufheight; i += 1)
            {
                *dest++ = *src;
                *dest++ = *(src + 1);
                *dest++ = *src;
                *dest++ = *(src + 1);
                *dest++ = *src;
                *dest++ = *(src + 1);
                *dest++ = 0;
                *dest++ = 0;
                src += 2;
            }
        }
        break;
        case V4L2_PIX_FMT_SBGGR16:
        {
            /* this is bgra , now I use unsigned short! */
            unsigned int i;
            unsigned short * src = (unsigned short *)rgb24_buffer;
            unsigned short * dest = (unsigned short *)colorBuffer;
            for (i = 0; i < bufwidth * bufheight; i += 1)
            {
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                *dest++ = *(src);
                *dest++ = 0;
                src += 3;
            }
        }
        default:
            ccvt_420p_bgr32(bufwidth, bufheight, (void *)yuvBuffer, (void *)colorBuffer);
            break;
    }
    return colorBuffer;
}
#endif

/* used for SER recorder */
unsigned char *V4L2_Builtin_Decoder::getRGBBuffer()
{
    //cerr << "in get color buffer " << endl;
    //IDLog("Decoder geRGBBuffer %s\n", (doCrop?"true":"false"));
    if (!rgb24_buffer)
        rgb24_buffer = new unsigned char[(bufwidth * bufheight) * 3];
    switch (fmt.fmt.pix.pixelformat)
    {
        case V4L2_PIX_FMT_GREY:
        case V4L2_PIX_FMT_JPEG:
        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
            ccvt_420p_rgb24(bufwidth, bufheight, (void *)yuvBuffer, (void *)rgb24_buffer);
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_VYUY:
        case V4L2_PIX_FMT_YVYU:
            //if (!colorBuffer) colorBuffer = new unsigned char[(bufwidth * bufheight) * 4];
            //ccvt_yuyv_bgr32(bufwidth, bufheight, yuyvBuffer, rgb24_buffer);
            //ccvt_bgr32_rgb24(bufwidth, bufheight, colorBuffer, (void*)rgb24_buffer);
            ccvt_yuyv_rgb24(bufwidth, bufheight, yuyvBuffer, (void *)rgb24_buffer);
            break;
        case V4L2_PIX_FMT_RGB24:
        case V4L2_PIX_FMT_RGB555:
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_SBGGR8:
        case V4L2_PIX_FMT_SRGGB8:
        case V4L2_PIX_FMT_SBGGR16:
            break;
        default:
            ccvt_420p_rgb24(bufwidth, bufheight, (void *)yuvBuffer, (void *)rgb24_buffer);
            break;
    }
    return rgb24_buffer;
}

int V4L2_Builtin_Decoder::getBpp()
{
    return (int)(bpp);
}

bool V4L2_Builtin_Decoder::issupportedformat(unsigned int format)
{
    //IDLog("Checking support for %c%c%c%c: %c\n", format&0xFF, (format>>8)&0xFF, (format>>16)&0xFF,(format>>24)&0xFF, (supported_formats.count(format) > 0)?'y':'n');
    return (supported_formats.count(format) > 0);
}

const std::vector<unsigned int> &V4L2_Builtin_Decoder::getsupportedformats()
{
    return vsuppformats;
}

void V4L2_Builtin_Decoder::init_supported_formats()
{
    /* RGB formats */
    // V4L2_PIX_FMT_RGB332  , // v4l2_fourcc('R', 'G', 'B', '1') /*  8  RGB-3-3-2     */
    // V4L2_PIX_FMT_RGB444  , // v4l2_fourcc('R', '4', '4', '4') /* 16  xxxxrrrr ggggbbbb */
    // V4L2_PIX_FMT_RGB555  , // v4l2_fourcc('R', 'G', 'B', 'O') /* 16  RGB-5-5-5     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_RGB555, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_RGB555, 8, true)));
    // V4L2_PIX_FMT_RGB565  , // v4l2_fourcc('R', 'G', 'B', 'P') /* 16  RGB-5-6-5     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_RGB565, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_RGB565, 8, true)));
    // V4L2_PIX_FMT_RGB555X , // v4l2_fourcc('R', 'G', 'B', 'Q') /* 16  RGB-5-5-5 BE  */
    // V4L2_PIX_FMT_RGB565X , // v4l2_fourcc('R', 'G', 'B', 'R') /* 16  RGB-5-6-5 BE  */
    // V4L2_PIX_FMT_BGR666  , // v4l2_fourcc('B', 'G', 'R', 'H') /* 18  BGR-6-6-6	  */
    // V4L2_PIX_FMT_BGR24   , // v4l2_fourcc('B', 'G', 'R', '3') /* 24  BGR-8-8-8     */
    // V4L2_PIX_FMT_RGB24   , // v4l2_fourcc('R', 'G', 'B', '3') /* 24  RGB-8-8-8     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_RGB24, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_RGB24, 8, true)));
    // V4L2_PIX_FMT_BGR32   , // v4l2_fourcc('B', 'G', 'R', '4') /* 32  BGR-8-8-8-8   */
    // V4L2_PIX_FMT_RGB32   , // v4l2_fourcc('R', 'G', 'B', '4') /* 32  RGB-8-8-8-8   */

    /* Grey formats */
    //V4L2_PIX_FMT_GREY    , // v4l2_fourcc('G', 'R', 'E', 'Y') /*  8  Greyscale     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_GREY, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_GREY, 8, true)));
    // V4L2_PIX_FMT_Y4      , // v4l2_fourcc('Y', '0', '4', ' ') /*  4  Greyscale     */
    // V4L2_PIX_FMT_Y6      , // v4l2_fourcc('Y', '0', '6', ' ') /*  6  Greyscale     */
    // V4L2_PIX_FMT_Y10     , // v4l2_fourcc('Y', '1', '0', ' ') /* 10  Greyscale     */
    // V4L2_PIX_FMT_Y12     , // v4l2_fourcc('Y', '1', '2', ' ') /* 12  Greyscale     */
    // V4L2_PIX_FMT_Y16     , // v4l2_fourcc('Y', '1', '6', ' ') /* 16  Greyscale     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_Y16, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_Y16, 16, true)));

    /* Grey bit-packed formats */
    // V4L2_PIX_FMT_Y10BPACK    , // v4l2_fourcc('Y', '1', '0', 'B') /* 10  Greyscale bit-packed */

    /* Palette formats */
    // V4L2_PIX_FMT_PAL8    , // v4l2_fourcc('P', 'A', 'L', '8') /*  8  8-bit palette */

    /* Chrominance formats */
    // V4L2_PIX_FMT_UV8     , // v4l2_fourcc('U', 'V', '8', ' ') /*  8  UV 4:4 */

    /* Luminance+Chrominance formats */
    // V4L2_PIX_FMT_YVU410  , // v4l2_fourcc('Y', 'V', 'U', '9') /*  9  YVU 4:1:0     */
    // V4L2_PIX_FMT_YVU420  , // v4l2_fourcc('Y', 'V', '1', '2') /* 12  YVU 4:2:0     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_YVU420, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_YVU420, 8, true)));
    // V4L2_PIX_FMT_YUYV    , // v4l2_fourcc('Y', 'U', 'Y', 'V') /* 16  YUV 4:2:2     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_YUYV, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_YUYV, 8, true)));
    // V4L2_PIX_FMT_YYUV    , // v4l2_fourcc('Y', 'Y', 'U', 'V') /* 16  YUV 4:2:2     */
    // V4L2_PIX_FMT_YVYU    , // v4l2_fourcc('Y', 'V', 'Y', 'U') /* 16 YVU 4:2:2 */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_YVYU, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_YVYU, 8, true)));
    // V4L2_PIX_FMT_UYVY    , // v4l2_fourcc('U', 'Y', 'V', 'Y') /* 16  YUV 4:2:2     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_UYVY, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_UYVY, 8, true)));
    // V4L2_PIX_FMT_VYUY    , // v4l2_fourcc('V', 'Y', 'U', 'Y') /* 16  YUV 4:2:2     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_VYUY, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_VYUY, 8, true)));
    // V4L2_PIX_FMT_YUV422P , // v4l2_fourcc('4', '2', '2', 'P') /* 16  YVU422 planar */
    // V4L2_PIX_FMT_YUV411P , // v4l2_fourcc('4', '1', '1', 'P') /* 16  YVU411 planar */
    // V4L2_PIX_FMT_Y41P    , // v4l2_fourcc('Y', '4', '1', 'P') /* 12  YUV 4:1:1     */
    // V4L2_PIX_FMT_YUV444  , // v4l2_fourcc('Y', '4', '4', '4') /* 16  xxxxyyyy uuuuvvvv */
    // V4L2_PIX_FMT_YUV555  , // v4l2_fourcc('Y', 'U', 'V', 'O') /* 16  YUV-5-5-5     */
    // V4L2_PIX_FMT_YUV565  , // v4l2_fourcc('Y', 'U', 'V', 'P') /* 16  YUV-5-6-5     */
    // V4L2_PIX_FMT_YUV32   , // v4l2_fourcc('Y', 'U', 'V', '4') /* 32  YUV-8-8-8-8   */
    // V4L2_PIX_FMT_YUV410  , // v4l2_fourcc('Y', 'U', 'V', '9') /*  9  YUV 4:1:0     */
    // V4L2_PIX_FMT_YUV420  , // v4l2_fourcc('Y', 'U', '1', '2') /* 12  YUV 4:2:0     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_YUV420, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_YUV420, 8, true)));
    // V4L2_PIX_FMT_HI240   , // v4l2_fourcc('H', 'I', '2', '4') /*  8  8-bit color   */
    // V4L2_PIX_FMT_HM12    , // v4l2_fourcc('H', 'M', '1', '2') /*  8  YUV 4:2:0 16x16 macroblocks */
    // V4L2_PIX_FMT_M420    , // v4l2_fourcc('M', '4', '2', '0') /* 12  YUV 4:2:0 2 lines y, 1 line uv interleaved */

    /* two planes -- one Y, one Cr + Cb interleaved  */
    // V4L2_PIX_FMT_NV12    , // v4l2_fourcc('N', 'V', '1', '2') /* 12  Y/CbCr 4:2:0  */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_NV12, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_NV12, 8, true)));
    // V4L2_PIX_FMT_NV21    , // v4l2_fourcc('N', 'V', '2', '1') /* 12  Y/CrCb 4:2:0  */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_NV21, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_NV21, 8, true)));
    // V4L2_PIX_FMT_NV16    , // v4l2_fourcc('N', 'V', '1', '6') /* 16  Y/CbCr 4:2:2  */
    // V4L2_PIX_FMT_NV61    , // v4l2_fourcc('N', 'V', '6', '1') /* 16  Y/CrCb 4:2:2  */
    // V4L2_PIX_FMT_NV24    , // v4l2_fourcc('N', 'V', '2', '4') /* 24  Y/CbCr 4:4:4  */
    // V4L2_PIX_FMT_NV42    , // v4l2_fourcc('N', 'V', '4', '2') /* 24  Y/CrCb 4:4:4  */

    /* two non contiguous planes - one Y, one Cr + Cb interleaved  */
    // V4L2_PIX_FMT_NV12M   , // v4l2_fourcc('N', 'M', '1', '2') /* 12  Y/CbCr 4:2:0  */
    // V4L2_PIX_FMT_NV21M   , // v4l2_fourcc('N', 'M', '2', '1') /* 21  Y/CrCb 4:2:0  */
    // V4L2_PIX_FMT_NV16M   , // v4l2_fourcc('N', 'M', '1', '6') /* 16  Y/CbCr 4:2:2  */
    // V4L2_PIX_FMT_NV61M   , // v4l2_fourcc('N', 'M', '6', '1') /* 16  Y/CrCb 4:2:2  */
    // V4L2_PIX_FMT_NV12MT  , // v4l2_fourcc('T', 'M', '1', '2') /* 12  Y/CbCr 4:2:0 64x32 macroblocks */
    // V4L2_PIX_FMT_NV12MT_16X16 , // v4l2_fourcc('V', 'M', '1', '2') /* 12  Y/CbCr 4:2:0 16x16 macroblocks */

    /* three non contiguous planes - Y, Cb, Cr */
    // V4L2_PIX_FMT_YUV420M , // v4l2_fourcc('Y', 'M', '1', '2') /* 12  YUV420 planar */
    // V4L2_PIX_FMT_YVU420M , // v4l2_fourcc('Y', 'M', '2', '1') /* 12  YVU420 planar */

    /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
    //  V4L2_PIX_FMT_SBGGR8  , // v4l2_fourcc('B', 'A', '8', '1') /*  8  BGBG.. GRGR.. */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_SBGGR8, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_SBGGR8, 8, false)));
    // V4L2_PIX_FMT_SGBRG8  , // v4l2_fourcc('G', 'B', 'R', 'G') /*  8  GBGB.. RGRG.. */
    // V4L2_PIX_FMT_SGRBG8  , // v4l2_fourcc('G', 'R', 'B', 'G') /*  8  GRGR.. BGBG.. */
    //  V4L2_PIX_FMT_SRGGB8  , // v4l2_fourcc('R', 'G', 'G', 'B') /*  8  RGRG.. GBGB.. */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_SRGGB8, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_SRGGB8, 8, false)));
    // V4L2_PIX_FMT_SBGGR10 , // v4l2_fourcc('B', 'G', '1', '0') /* 10  BGBG.. GRGR.. */
    // V4L2_PIX_FMT_SGBRG10 , // v4l2_fourcc('G', 'B', '1', '0') /* 10  GBGB.. RGRG.. */
    // V4L2_PIX_FMT_SGRBG10 , // v4l2_fourcc('B', 'A', '1', '0') /* 10  GRGR.. BGBG.. */
    // V4L2_PIX_FMT_SRGGB10 , // v4l2_fourcc('R', 'G', '1', '0') /* 10  RGRG.. GBGB.. */
    // V4L2_PIX_FMT_SBGGR12 , // v4l2_fourcc('B', 'G', '1', '2') /* 12  BGBG.. GRGR.. */
    // V4L2_PIX_FMT_SGBRG12 , // v4l2_fourcc('G', 'B', '1', '2') /* 12  GBGB.. RGRG.. */
    // V4L2_PIX_FMT_SGRBG12 , // v4l2_fourcc('B', 'A', '1', '2') /* 12  GRGR.. BGBG.. */
    // V4L2_PIX_FMT_SRGGB12 , // v4l2_fourcc('R', 'G', '1', '2') /* 12  RGRG.. GBGB.. */
    /* 10bit raw bayer a-law compressed to 8 bits */
    // V4L2_PIX_FMT_SBGGR10ALAW8 , // v4l2_fourcc('a', 'B', 'A', '8')
    // V4L2_PIX_FMT_SGBRG10ALAW8 , // v4l2_fourcc('a', 'G', 'A', '8')
    // V4L2_PIX_FMT_SGRBG10ALAW8 , // v4l2_fourcc('a', 'g', 'A', '8')
    // V4L2_PIX_FMT_SRGGB10ALAW8 , // v4l2_fourcc('a', 'R', 'A', '8')
    /* 10bit raw bayer DPCM compressed to 8 bits */
    // V4L2_PIX_FMT_SBGGR10DPCM8 , // v4l2_fourcc('b', 'B', 'A', '8')
    // V4L2_PIX_FMT_SGBRG10DPCM8 , // v4l2_fourcc('b', 'G', 'A', '8')
    // V4L2_PIX_FMT_SGRBG10DPCM8 , // v4l2_fourcc('B', 'D', '1', '0')
    // V4L2_PIX_FMT_SRGGB10DPCM8 , // v4l2_fourcc('b', 'R', 'A', '8')
    /*
     * 10bit raw bayer, expanded to 16 bits
     * xxxxrrrrrrrrrrxxxxgggggggggg xxxxggggggggggxxxxbbbbbbbbbb...
     */
    // V4L2_PIX_FMT_SBGGR16 , // v4l2_fourcc('B', 'Y', 'R', '2') /* 16  BGBG.. GRGR.. */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_SBGGR16, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_SBGGR16, 16, false)));

    /* compressed formats */
    // V4L2_PIX_FMT_MJPEG    , // v4l2_fourcc('M', 'J', 'P', 'G') /* Motion-JPEG   */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_MJPEG, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_MJPEG, 8, false)));
    // V4L2_PIX_FMT_JPEG     , // v4l2_fourcc('J', 'P', 'E', 'G') /* JFIF JPEG     */
    supported_formats.insert(
        std::make_pair(V4L2_PIX_FMT_JPEG, new V4L2_Builtin_Decoder::format(V4L2_PIX_FMT_JPEG, 8, false)));
    // V4L2_PIX_FMT_DV       , // v4l2_fourcc('d', 'v', 's', 'd') /* 1394          */
    // V4L2_PIX_FMT_MPEG     , // v4l2_fourcc('M', 'P', 'E', 'G') /* MPEG-1/2/4 Multiplexed */
    // V4L2_PIX_FMT_H264     , // v4l2_fourcc('H', '2', '6', '4') /* H264 with start codes */
    // V4L2_PIX_FMT_H264_NO_SC , // v4l2_fourcc('A', 'V', 'C', '1') /* H264 without start codes */
    // V4L2_PIX_FMT_H264_MVC , // v4l2_fourcc('M', '2', '6', '4') /* H264 MVC */
    // V4L2_PIX_FMT_H263     , // v4l2_fourcc('H', '2', '6', '3') /* H263          */
    // V4L2_PIX_FMT_MPEG1    , // v4l2_fourcc('M', 'P', 'G', '1') /* MPEG-1 ES     */
    // V4L2_PIX_FMT_MPEG2    , // v4l2_fourcc('M', 'P', 'G', '2') /* MPEG-2 ES     */
    // V4L2_PIX_FMT_MPEG4    , // v4l2_fourcc('M', 'P', 'G', '4') /* MPEG-4 part 2 ES */
    // V4L2_PIX_FMT_XVID     , // v4l2_fourcc('X', 'V', 'I', 'D') /* Xvid           */
    // V4L2_PIX_FMT_VC1_ANNEX_G , // v4l2_fourcc('V', 'C', '1', 'G') /* SMPTE 421M Annex G compliant stream */
    // V4L2_PIX_FMT_VC1_ANNEX_L , // v4l2_fourcc('V', 'C', '1', 'L') /* SMPTE 421M Annex L compliant stream */
    // V4L2_PIX_FMT_VP8      , // v4l2_fourcc('V', 'P', '8', '0') /* VP8 */

    /*  Vendor-specific formats   */
    // V4L2_PIX_FMT_CPIA1    , // v4l2_fourcc('C', 'P', 'I', 'A') /* cpia1 YUV */
    // V4L2_PIX_FMT_WNVA     , // v4l2_fourcc('W', 'N', 'V', 'A') /* Winnov hw compress */
    // V4L2_PIX_FMT_SN9C10X  , // v4l2_fourcc('S', '9', '1', '0') /* SN9C10x compression */
    // V4L2_PIX_FMT_SN9C20X_I420 , // v4l2_fourcc('S', '9', '2', '0') /* SN9C20x YUV 4:2:0 */
    // V4L2_PIX_FMT_PWC1     , // v4l2_fourcc('P', 'W', 'C', '1') /* pwc older webcam */
    // V4L2_PIX_FMT_PWC2     , // v4l2_fourcc('P', 'W', 'C', '2') /* pwc newer webcam */
    // V4L2_PIX_FMT_ET61X251 , // v4l2_fourcc('E', '6', '2', '5') /* ET61X251 compression */
    // V4L2_PIX_FMT_SPCA501  , // v4l2_fourcc('S', '5', '0', '1') /* YUYV per line */
    // V4L2_PIX_FMT_SPCA505  , // v4l2_fourcc('S', '5', '0', '5') /* YYUV per line */
    // V4L2_PIX_FMT_SPCA508  , // v4l2_fourcc('S', '5', '0', '8') /* YUVY per line */
    // V4L2_PIX_FMT_SPCA561  , // v4l2_fourcc('S', '5', '6', '1') /* compressed GBRG bayer */
    // V4L2_PIX_FMT_PAC207   , // v4l2_fourcc('P', '2', '0', '7') /* compressed BGGR bayer */
    // V4L2_PIX_FMT_MR97310A , // v4l2_fourcc('M', '3', '1', '0') /* compressed BGGR bayer */
    // V4L2_PIX_FMT_JL2005BCD , // v4l2_fourcc('J', 'L', '2', '0') /* compressed RGGB bayer */
    // V4L2_PIX_FMT_SN9C2028 , // v4l2_fourcc('S', 'O', 'N', 'X') /* compressed GBRG bayer */
    // V4L2_PIX_FMT_SQ905C   , // v4l2_fourcc('9', '0', '5', 'C') /* compressed RGGB bayer */
    // V4L2_PIX_FMT_PJPG     , // v4l2_fourcc('P', 'J', 'P', 'G') /* Pixart 73xx JPEG */
    // V4L2_PIX_FMT_OV511    , // v4l2_fourcc('O', '5', '1', '1') /* ov511 JPEG */
    // V4L2_PIX_FMT_OV518    , // v4l2_fourcc('O', '5', '1', '8') /* ov518 JPEG */
    // V4L2_PIX_FMT_STV0680  , // v4l2_fourcc('S', '6', '8', '0') /* stv0680 bayer */
    // V4L2_PIX_FMT_TM6000   , // v4l2_fourcc('T', 'M', '6', '0') /* tm5600/tm60x0 */
    // V4L2_PIX_FMT_CIT_YYVYUY , // v4l2_fourcc('C', 'I', 'T', 'V') /* one line of Y then 1 line of VYUY */
    // V4L2_PIX_FMT_KONICA420  , // v4l2_fourcc('K', 'O', 'N', 'I') /* YUV420 planar in blocks of 256 pixels */
    // V4L2_PIX_FMT_JPGL	, // v4l2_fourcc('J', 'P', 'G', 'L') /* JPEG-Lite */
    // V4L2_PIX_FMT_SE401      , // v4l2_fourcc('S', '4', '0', '1') /* se401 janggu compressed rgb */
    // V4L2_PIX_FMT_S5C_UYVY_JPG  // v4l2_fourcc('S', '5', 'C', 'I') /* S5C73M3 interleaved UYVY/JPEG */

    for (std::map<unsigned int, struct format *>::iterator it = supported_formats.begin();
         it != supported_formats.end(); ++it)
        vsuppformats.push_back(it->first);
}
