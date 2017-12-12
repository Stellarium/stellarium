/*
    Copyright (C) 2017 by Jasem Mutlaq <mutlaqja@ikarustech.com>

    Theora Recorder

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

#include "theorarecorder.h"
#include "jpegutils.h"

#define _FILE_OFFSET_BITS 64

#include <ctime>
#include <cerrno>
#include <cstring>
#include <sys/time.h>

#define ERRMSGSIZ 1024

static int ilog(unsigned _v)
{
    int ret;
    for(ret=0;_v;ret++)_v>>=1;
    return ret;
}

namespace INDI
{

TheoraRecorder::TheoraRecorder()
{
    name = "OGV";
    isRecordingActive = false;

    ycbcr[0].data = nullptr;
    ycbcr[1].data = nullptr;
    ycbcr[2].data = nullptr;
}

TheoraRecorder::~TheoraRecorder()
{
    delete [] ycbcr[0].data;
    delete [] ycbcr[1].data;
    delete [] ycbcr[2].data;

    th_encode_free(td);
}

bool TheoraRecorder::setPixelFormat(INDI_PIXEL_FORMAT pixelFormat, uint8_t pixelDepth)
{
    m_PixelFormat = pixelFormat;
    m_PixelDepth  = pixelDepth;
    return true;
}

bool TheoraRecorder::setSize(uint16_t width, uint16_t height)
{
    if (isRecordingActive)
        return false;

    rawWidth  = width;
    rawHeight = height;

    return allocateBuffers();
}

bool TheoraRecorder::allocateBuffers()
{
    /* Must hold: yuv_w >= w */
    uint16_t yuv_w = (rawWidth + 15) & ~15;
    /* Must hold: yuv_h >= h */
    uint16_t yuv_h = (rawHeight + 15) & ~15;

    /* Do we need to allocate a buffer */
    if (!ycbcr[0].data || yuv_w != ycbcr[0].width || yuv_h != ycbcr[0].height)
    {
        ycbcr[0].width = yuv_w;
        ycbcr[0].height = yuv_h;
        ycbcr[0].stride = yuv_w;
#if 0
        if (m_PixelFormat == INDI_MONO)
        {
            ycbcr[1].width = 0;
            ycbcr[1].stride = 0;
            ycbcr[1].height = 0;
            ycbcr[2].width = 0;
            ycbcr[2].stride = 0;
            ycbcr[2].height = 0;
        }
        else
        {
#endif
            ycbcr[1].width = (chroma_format == TH_PF_444) ? yuv_w : (yuv_w >> 1);
            ycbcr[1].stride = ycbcr[1].width;
            ycbcr[1].height = (chroma_format == TH_PF_420) ? (yuv_h >> 1) : yuv_h;
            ycbcr[2].width = ycbcr[1].width;
            ycbcr[2].stride = ycbcr[1].stride;
            ycbcr[2].height = ycbcr[1].height;
#if 0
        }
#endif

        delete [] ycbcr[0].data;
        delete [] ycbcr[1].data;
        delete [] ycbcr[2].data;

        ycbcr[0].data = new uint8_t[ycbcr[0].stride * ycbcr[0].height];
        ycbcr[1].data = new uint8_t[ycbcr[1].stride * ycbcr[1].height];
        ycbcr[2].data = new uint8_t[ycbcr[2].stride * ycbcr[2].height];

#if 0
        ycbcr[0].data = new uint8_t[ycbcr[0].stride * ycbcr[0].height];
        ycbcr[1].data = (m_PixelFormat == INDI_MONO) ? 0 : new uint8_t[ycbcr[1].stride * ycbcr[1].height];
        ycbcr[2].data = (m_PixelFormat == INDI_MONO) ? 0 : new uint8_t[ycbcr[2].stride * ycbcr[2].height];
#endif
    }

    return true;
}

bool TheoraRecorder::open(const char *filename, char *errmsg)
{
    if (isRecordingActive)
        return false;

    if(soft_target)
    {
        if(video_rate<=0)
        {
            snprintf(errmsg, ERRMSGSIZ, "Soft rate target requested without a bitrate.");
            return false;
        }

        if(video_quality==-1)
            video_quality=0;
    }
    else
    {
        if(video_rate>0)
            video_quality=0;
        if(video_quality==-1)
            video_quality=48;
    }

    if(keyframe_frequency<=0)
    {
        /*Use a default keyframe frequency of 64 for 1-pass (streaming) mode, and
         256 for two-pass mode.*/
        keyframe_frequency=twopass?256:64;
    }

    ogg_fp = fopen(filename, "wb");
    if(!ogg_fp)
    {
        snprintf(errmsg, ERRMSGSIZ, "%s: error: could not open output file", filename);
        return false;
    }

    srand(time(NULL));
    if(ogg_stream_init(&ogg_os, rand()))
    {
        snprintf(errmsg, ERRMSGSIZ, "%s: error: could not create ogg stream state", filename);
        return false;
    }

    th_info_init(&ti);
    ti.frame_width = ((rawWidth + 15) >>4)<<4;
    ti.frame_height = ((rawHeight + 15)>>4)<<4;
    ti.pic_width = rawWidth;
    ti.pic_height = rawHeight;
    ti.pic_x = 0;
    ti.pic_y = 0;
    frac(m_FPS, video_fps_numerator, video_fps_denominator);
    ti.fps_numerator = video_fps_numerator;
    ti.fps_denominator = video_fps_denominator;
    ti.aspect_numerator = video_aspect_numerator;
    ti.aspect_denominator = video_aspect_denominator;
    ti.colorspace = TH_CS_UNSPECIFIED;
    ti.pixel_fmt = static_cast<th_pixel_fmt>(chroma_format);
    ti.target_bitrate = video_rate;
    ti.quality = video_quality;
    ti.keyframe_granule_shift=ilog(keyframe_frequency-1);

    td=th_encode_alloc(&ti);
    th_info_clear(&ti);
    /* setting just the granule shift only allows power-of-two keyframe
       spacing.  Set the actual requested spacing. */
    int ret=th_encode_ctl(td,TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, &keyframe_frequency,sizeof(keyframe_frequency-1));
    if(ret<0)
    {
        snprintf(errmsg, ERRMSGSIZ, "Could not set keyframe interval to %d.",(int)keyframe_frequency);
    }

    if(vp3_compatible)
    {
        ret=th_encode_ctl(td,TH_ENCCTL_SET_VP3_COMPATIBLE,&vp3_compatible,  sizeof(vp3_compatible));
        if(ret<0||!vp3_compatible)
        {
            snprintf(errmsg, ERRMSGSIZ, "Could not enable strict VP3 compatibility.");
        }
    }

    if(soft_target)
    {
        /* reverse the rate control flags to favor a 'long time' strategy */
        int arg = TH_RATECTL_CAP_UNDERFLOW;
        ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_FLAGS,&arg,sizeof(arg));
        if(ret<0)
            snprintf(errmsg, ERRMSGSIZ, "Could not set encoder flags for soft-target");
        /* Default buffer control is overridden on two-pass */
        if(!twopass&&buf_delay<0)
        {
            if((keyframe_frequency*7>>1) > 5*video_fps_numerator/video_fps_denominator)
                arg=keyframe_frequency*7>>1;
            else
                arg=5*video_fps_numerator/video_fps_denominator;
            ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_BUFFER,&arg,sizeof(arg));
            if(ret<0)
                snprintf(errmsg, ERRMSGSIZ, "Could not set rate control buffer for soft-target");
        }
    }

    /* set up two-pass if needed */
    if(passno==1)
    {
        unsigned char *buffer;
        int bytes;
        bytes=th_encode_ctl(td,TH_ENCCTL_2PASS_OUT,&buffer,sizeof(buffer));
        if(bytes<0)
        {
            //IDLog("Could not set up the first pass of two-pass mode.");
            //IDLog("Did you remember to specify an estimated bitrate?");
            //exit(1);
            return false;
        }
        /*Perform a seek test to ensure we can overwrite this placeholder data at
         the end; this is better than letting the user sit through a whole
         encode only to find out their pass 1 file is useless at the end.*/
        if(fseek(twopass_file,0,SEEK_SET)<0)
        {
            //IDLog("Unable to seek in two-pass data file.");
            //exit(1);
            return false;
        }
        if(fwrite(buffer,1,bytes,twopass_file)< static_cast<size_t>(bytes))
        {
            IDLog("Unable to write to two-pass data file.");
            return false;
            //exit(1);
        }
        fflush(twopass_file);
    }
    if(passno==2){
        /*Enable the second pass here.
        We make this call just to set the encoder into 2-pass mode, because
         by default enabling two-pass sets the buffer delay to the whole file
         (because there's no way to explicitly request that behavior).
        If we waited until we were actually encoding, it would overwite our
         settings.*/
        if(th_encode_ctl(td,TH_ENCCTL_2PASS_IN,NULL,0)<0)
        {
            snprintf(errmsg, ERRMSGSIZ, "Could not set up the second pass of two-pass mode.");
            return false;
        }
        if(twopass==3)
        {
            if(fseek(twopass_file,0,SEEK_SET)<0)
            {
                snprintf(errmsg, ERRMSGSIZ, "Unable to seek in two-pass data file.");
                return false;
            }
        }
    }
    /*Now we can set the buffer delay if the user requested a non-default one
       (this has to be done after two-pass is enabled).*/
    if(passno!=1&&buf_delay>=0)
    {
        ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_BUFFER,   &buf_delay,sizeof(buf_delay));
        if(ret<0)
        {
            snprintf(errmsg, ERRMSGSIZ, "Warning: could not set desired buffer delay.");
        }
    }

    /* write the bitstream header packets with proper page interleave */
    th_comment_init(&tc);
    /* first packet will get its own page automatically */
    if(th_encode_flushheader(td,&tc,&op)<=0)
    {
        snprintf(errmsg, ERRMSGSIZ, "Internal Theora library error.");
        return false;
    }

    th_comment_clear(&tc);
    if(passno!=1)
    {
        ogg_stream_packetin(&ogg_os,&op);
        if(ogg_stream_pageout(&ogg_os,&og)!=1)
        {
            snprintf(errmsg, ERRMSGSIZ, "Internal Ogg library error.");
            return false;
        }

        fwrite(og.header,1,og.header_len,ogg_fp);
        fwrite(og.body,1,og.body_len,ogg_fp);
    }

    /* create the remaining theora headers */
    for(;;)
    {
        ret=th_encode_flushheader(td,&tc,&op);
        if(ret<0)
        {
            snprintf(errmsg, ERRMSGSIZ,"Internal Theora library error.");
            return false;
        }
        else if(!ret)
            break;
        if(passno!=1)
            ogg_stream_packetin(&ogg_os,&op);
    }
    /* Flush the rest of our headers. This ensures
           the actual data in each stream will start
           on a new page, as per spec. */
    if(passno!=1)
    {
        for(;;)
        {
            int result = ogg_stream_flush(&ogg_os,&og);
            if(result<0)
            {
                /* can't get here */
                snprintf(errmsg, ERRMSGSIZ,"Internal Ogg library error.");
                return false;
            }
            if(result==0)
                break;
            fwrite(og.header,1,og.header_len,ogg_fp);
            fwrite(og.body,1,og.body_len,ogg_fp);
        }
    }

    isRecordingActive = true;

    return true;
}

bool TheoraRecorder::close()
{
    theora_write_frame(1);

    if(passno==1)
    {
        /* need to read the final (summary) packet */
        unsigned char *buffer;
        int bytes = th_encode_ctl(td, TH_ENCCTL_2PASS_OUT, &buffer, sizeof(buffer));
        if(bytes<0)
        {
            IDLog("Could not read two-pass summary data from encoder.");
            return false;
        }
        if(fseek(twopass_file,0,SEEK_SET)<0)
        {
            IDLog("Unable to seek in two-pass data file.");
            return false;
        }

        if(fwrite(buffer,1,bytes,twopass_file)< static_cast<size_t>(bytes))
        {
            IDLog("Unable to write to two-pass data file.");
            return false;
        }
        fflush(twopass_file);
    }

    /*th_encode_free(td);
    free(ycbcr[0].data);
    ycbcr[0].data = nullptr;
    free(ycbcr[1].data);
    free(ycbcr[2].data);*/

    if(ogg_stream_flush(&ogg_os, &og))
    {
        fwrite(og.header, og.header_len, 1, ogg_fp);
        fwrite(og.body, og.body_len, 1, ogg_fp);
    }

    if(ogg_fp)
    {
        fflush(ogg_fp);
        fclose(ogg_fp);
    }

    ogg_stream_clear(&ogg_os);
    if(twopass_file)
        fclose(twopass_file);

    isRecordingActive = false;
    return true;
}

bool TheoraRecorder::writeFrame(const uint8_t *frame, uint32_t nbytes)
{
    if (!isRecordingActive)
        return false;

    if (m_PixelFormat == INDI_MONO)
    {
        memcpy(ycbcr[0].data, frame, ycbcr[0].stride * ycbcr[0].height);
        // Cb and Cr values to 0x80 (128) for grayscale image
        memset(ycbcr[1].data, 0x80, ycbcr[1].stride * ycbcr[1].height);
        memset(ycbcr[2].data, 0x80, ycbcr[2].stride * ycbcr[2].height);
    }
    else if (m_PixelFormat == INDI_JPG)
    {
        decode_jpeg_raw((const_cast<uint8_t *>(frame)), nbytes, 0, 0, rawWidth, rawHeight, ycbcr[0].data, ycbcr[1].data, ycbcr[2].data );
    }
    else
        return false;

    theora_write_frame(0);

    return true;
}

# if 0
bool TheoraRecorder::writeFrameMono(uint8_t *frame)
{
    if (isStreamingActive == false &&
            (subX > 0 || subY > 0 || subW != rawWidth || subH != rawHeight))
    {
        int offset = ((rawWidth * subY) + subX);

        uint8_t *srcBuffer  = frame + offset;
        uint8_t *destBuffer = frame;
        int imageWidth      = subW;
        int imageHeight     = subH;

        for (int i = 0; i < imageHeight; i++)
            memcpy(destBuffer + i * imageWidth, srcBuffer + rawWidth * i, imageWidth);
    }

    return writeFrame(frame);
}

bool TheoraRecorder::writeFrameColor(uint8_t *frame)
{
    if (isStreamingActive == false &&
            (subX > 0 || subY > 0 || subW != rawWidth || subH != rawHeight))
    {
        int offset = ((rawWidth * subY) + subX);

        uint8_t *srcBuffer  = frame + offset * 3;
        uint8_t *destBuffer = frame;
        int imageWidth      = subW;
        int imageHeight     = subH;

        // RGB
        for (int i = 0; i < imageHeight; i++)
            memcpy(destBuffer + i * imageWidth * 3, srcBuffer + rawWidth * 3 * i, imageWidth * 3);
    }

    return writeFrame(frame);
}
#endif



int TheoraRecorder::theora_write_frame(int last)
{
    ogg_packet op;
    ogg_page og;

    int rc = -1;

    if( (rc = th_encode_ycbcr_in(td, ycbcr)) )
    {
      IDLog("error: could not encode frame %d", rc);
      return rc;
    }

    /* in two-pass mode's first pass we need to extract and save the pass data */
    if(passno==1)
    {
      unsigned char *buffer;
      int bytes = th_encode_ctl(td, TH_ENCCTL_2PASS_OUT, &buffer, sizeof(buffer));

      if(bytes<0)
      {
        IDLog("Could not read two-pass data from encoder.");
        return 1;
      }

      if(fwrite(buffer,1,bytes,twopass_file) < static_cast<size_t>(bytes))
      {
        IDLog("Unable to write to two-pass data file.");
        return 1;
      }

      fflush(twopass_file);
    }

    if(!th_encode_packetout(td, last, &op))
    {
      IDLog("error: could not read packets");
      return 1;
    }

    if (passno!=1)
    {
      ogg_stream_packetin(&ogg_os, &op);
      while(ogg_stream_pageout(&ogg_os, &og))
      {
        fwrite(og.header, og.header_len, 1, ogg_fp);
        fwrite(og.body, og.body_len, 1, ogg_fp);
      }
    }

    return 0;
}

/*
** find rational approximation to given real number
** David Eppstein / UC Irvine / 8 Aug 1993
**
** With corrections from Arno Formella, May 2008
**
** usage: a.out r d
**   r is real number to approx
**   d is the maximum denominator allowed
**
** based on the theory of continued fractions
** if x = a1 + 1/(a2 + 1/(a3 + 1/(a4 + ...)))
** then best approximation is found by truncating this series
** (with some adjustments in the last term).
**
** Note the fraction can be recovered as the first column of the matrix
**  ( a1 1 ) ( a2 1 ) ( a3 1 ) ...
**  ( 1  0 ) ( 1  0 ) ( 1  0 )
** Instead of keeping the sequence of continued fraction terms,
** we just keep the last partial product of these matrices.
*/
bool TheoraRecorder::frac(double fps, uint32_t &num, uint32_t &den)
{
    long m[2][2];
    double x, startx;
    long maxden;
    long ai;

    startx = x = fps;
    maxden = 100;

    /* initialize matrix */
    m[0][0] = m[1][1] = 1;
    m[0][1] = m[1][0] = 0;

    /* loop finding terms until denom gets too big */
    while (m[1][0] *  ( ai = (long)x ) + m[1][1] <= maxden)
    {
        long t;
        t = m[0][0] * ai + m[0][1];
        m[0][1] = m[0][0];
        m[0][0] = t;
        t = m[1][0] * ai + m[1][1];
        m[1][1] = m[1][0];
        m[1][0] = t;
            if(x==(double)ai)
                break;     // AF: division by zero
        x = 1/(x - (double) ai);
            if(x>(double)0x7FFFFFFF)
                break;  // AF: representation failure
    }

    num = m[0][0];
    den = m[1][0];
    return true;

    /* now remaining x is between 0 and 1/ai */
    /* approx as either 0 or 1/m where m is max that will fit in maxden */
    /* first try zero */
    //printf("%ld/%ld, error = %e\n", m[0][0], m[1][0], startx - ((double) m[0][0] / (double) m[1][0]));

    /* now try other possibility */
    //ai = (maxden - m[1][1]) / m[1][0];
    //m[0][0] = m[0][0] * ai + m[0][1];
    //m[1][0] = m[1][0] * ai + m[1][1];
    //printf("%ld/%ld, error = %e\n", m[0][0], m[1][0],
    //   startx - ((double) m[0][0] / (double) m[1][0]));
}
}
