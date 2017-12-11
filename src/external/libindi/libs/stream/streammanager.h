/*
    Copyright (C) 2015 by Jasem Mutlaq <mutlaqja@ikarustech.com>
    Copyright (C) 2014 by geehalel <geehalel@gmail.com>

    Stream Recorder

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

#include "indidevapi.h"
#include "recorder/recordermanager.h"
#include "encoder/encodermanager.h"

#include <string>
#include <map>
#include <sys/time.h>

#include <stdint.h>

/**
 * \class StreamRecorder
   \brief Class to provide video streaming and recording functionality.

   INDI::CCD can utilize this class to add streaming and recording functionality to their driver. Currently, only SER recording backend is supported.

   \example Check V4L2 CCD and ZWO ASI drivers for example implementations.

\author Jean-Luc Geehalel, Jasem Mutlaq
*/
namespace INDI
{

class CCD;

class StreamManager
{
  public:
    enum
    {
        RECORD_ON,
        RECORD_TIME,
        RECORD_FRAME,
        RECORD_OFF
    };

    StreamManager(CCD *mainCCD);
    ~StreamManager();

    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);

    virtual bool initProperties();
    virtual bool updateProperties();

    virtual bool saveConfigItems(FILE *fp);

    /**
         * @brief newFrame CCD drivers call this function when a new frame is received. It is then streamed, or recorded, or both according to the settings in the streamer.
         */
    void newFrame(const uint8_t *buffer, uint32_t nbytes);

    /**
         * @brief setStream Enables (starts) or disables (stops) streaming.
         * @param enable True to enable, false to disable
         * @return True if operation is successful, false otherwise.
         */
    bool setStream(bool enable);

    RecorderInterface *getRecorder() { return recorder; }
    bool isDirectRecording() { return direct_record; }
    bool isStreaming() { return m_isStreaming; }
    bool isRecording() { return m_isRecording; }
    bool isBusy() { return (isStreaming() || isRecording()); }
    uint8_t getTargetFPS() { return static_cast<uint8_t>(StreamOptionsN[OPTION_TARGET_FPS].value); }

    uint8_t *getDownscaleBuffer() { return downscaleBuffer; }
    uint32_t getDownscaleBufferSize() { return downscaleBufferSize; }

    const char *getDeviceName();

    void setSize(uint16_t width, uint16_t height);
    bool setPixelFormat(INDI_PIXEL_FORMAT pixelFormat, uint8_t pixelDepth=8);
    void getStreamFrame(uint16_t *x, uint16_t *y, uint16_t *w, uint16_t *h);
    bool close();

  protected:
    CCD *currentCCD = nullptr;

  private:
    /* Utility for record file */
    int mkpath(std::string s, mode_t mode);
    std::string expand(std::string fname, const std::map<std::string, std::string> &patterns);

    bool startRecording();
    bool stopRecording();

    /**
     * @brief uploadStream Upload frame to client using the selected encoder
     * @param buffer pointer to frame image buffer
     * @param nbytes size of frame in bytes
     * @return True if frame is encoded and sent to client, false otherwise.
     */
    bool uploadStream(const uint8_t *buffer, uint32_t nbytes);

    /**
         * @brief recordStream Calls the backend recorder to record a single frame.
         * @param deltams time in milliseconds since last frame
         */
    bool recordStream(const uint8_t *buffer, uint32_t nbytes, double deltams);

    /* Stream switch */
    ISwitch StreamS[2];
    ISwitchVectorProperty StreamSP;

    /* Record switch */
    ISwitch RecordStreamS[4];
    ISwitchVectorProperty RecordStreamSP;

    /* Record File Info */
    IText RecordFileT[2];
    ITextVectorProperty RecordFileTP;

    /* Streaming Options */
    INumber StreamOptionsN[2];
    INumberVectorProperty StreamOptionsNP;
    enum { OPTION_TARGET_FPS, OPTION_RATE_DIVISOR};

    /* Measured FPS */
    INumber FpsN[2];
    INumberVectorProperty FpsNP;
    enum { FPS_INSTANT, FPS_AVERAGE };

    /* Record Options */
    INumber RecordOptionsN[2];
    INumberVectorProperty RecordOptionsNP;

    // Stream Frame
    INumberVectorProperty StreamFrameNP;
    INumber StreamFrameN[4];

    /* BLOBs */
    IBLOBVectorProperty *imageBP;
    IBLOB *imageB;

    // Encoder Selector. It's static now but should this implemented as plugin interface?
    ISwitch EncoderS[2];
    ISwitchVectorProperty EncoderSP;
    enum { ENCODER_RAW, ENCODER_MJPEG };

    // Recorder Selector. Static but should be implmeneted as a dynamic plugin interface
    ISwitch RecorderS[2];
    ISwitchVectorProperty RecorderSP;
    enum { RECORDER_RAW, RECORDER_OGV };

    bool m_isStreaming;
    bool m_isRecording;

    int streamframeCount;
    int recordframeCount;
    double recordDuration;    

    // Recorder
    RecorderManager *recorderManager = nullptr;
    RecorderInterface *recorder = nullptr;
    bool direct_record;
    std::string recordfiledir, recordfilename; /* in case we should move it */

    // Encoders
    EncoderManager *encoderManager = nullptr;
    EncoderInterface *encoder = nullptr;

    // Measure FPS
    // timer_t fpstimer;
    // struct itimerspec tframe1, tframe2;
    // use bsd timers
    struct itimerval tframe1, tframe2;
    double mssum, framecountsec;

    INDI_PIXEL_FORMAT m_PixelFormat;
    uint8_t m_PixelDepth;
    uint16_t rawWidth=0, rawHeight=0;

    // Downscale buffer for streaming
    uint8_t *downscaleBuffer = nullptr;
    uint32_t downscaleBufferSize=0;
};
}
