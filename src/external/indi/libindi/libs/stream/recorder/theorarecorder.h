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

#pragma once

#include "recorderinterface.h"

#include <ogg/ogg.h>
#include <theora/theoraenc.h>

#include <cstdint>
#include <stdio.h>

namespace INDI
{

class TheoraRecorder : public RecorderInterface
{
  public:
    TheoraRecorder();
    virtual ~TheoraRecorder();

    virtual const char *getExtension() { return ".ogv"; }
    virtual bool setPixelFormat(INDI_PIXEL_FORMAT pixelFormat, uint8_t pixelDepth);
    virtual bool setSize(uint16_t width, uint16_t height);        
    virtual bool open(const char *filename, char *errmsg);
    virtual bool close();
    virtual bool writeFrame(const uint8_t *frame, uint32_t nbytes);
    virtual void setStreamEnabled(bool enable) { isStreamingActive = enable; }

  protected:
    bool isRecordingActive = false, isStreamingActive = false;
    uint32_t number_of_planes;
    uint16_t rawWidth = 0, rawHeight = 0;
    std::vector<uint64_t> frameStamps;
    INDI_PIXEL_FORMAT m_PixelFormat;
    uint8_t m_PixelDepth=8;

  private:
    bool allocateBuffers();
    //int theora_write_frame(th_ycbcr_buffer ycbcr, int last);
    int theora_write_frame(int last);
    bool frac(double fps, uint32_t &num, uint32_t &den);

    th_ycbcr_buffer ycbcr;
    ogg_uint32_t video_fps_numerator = 24;
    ogg_uint32_t video_fps_denominator = 1;
    ogg_uint32_t video_aspect_numerator = 0;
    ogg_uint32_t video_aspect_denominator = 0;
    int video_rate = -1;
    int video_quality = -1;
    int soft_target=0;
    ogg_uint32_t keyframe_frequency=0;
    int buf_delay=-1;
    int vp3_compatible=0;
    int chroma_format = TH_PF_420;

    FILE *twopass_file = nullptr;
    int twopass=0;
    int passno=0;

    FILE *ogg_fp = nullptr;
    ogg_stream_state ogg_os;
    ogg_packet op;
    ogg_page og;

    th_enc_ctx      *td = nullptr;
    th_info          ti;
    th_comment       tc;
};
}
