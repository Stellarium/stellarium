/*
 QHY INDI Driver

 Copyright (C) 2014 Jasem Mutlaq (mutlaqja@ikarustech.com)
 Copyright (C) 2014 Zhirong Li (lzr@qhyccd.com)

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

#include "qhyccd.h"

#include <indiccd.h>
#include <indifilterinterface.h>

#include <pthread.h>

#define DEVICE struct usb_device *

class QHYCCD : public INDI::CCD, public INDI::FilterInterface
{
  public:
    QHYCCD(const char *name);
    virtual ~QHYCCD();

    virtual const char *getDefaultName() override;

    virtual bool initProperties() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool updateProperties() override;

    virtual bool Connect() override;
    virtual bool Disconnect() override;

    virtual int SetTemperature(double temperature) override;
    virtual bool StartExposure(float duration) override;
    virtual bool AbortExposure() override;

    virtual void debugTriggered(bool enable) override;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

    static void *streamVideoHelper(void *context);
    void *streamVideo();

  protected:
    // Misc.
    virtual void TimerHit() override;
    virtual bool saveConfigItems(FILE *fp) override;

    // CCD
    virtual bool UpdateCCDFrame(int x, int y, int w, int h) override;
    virtual bool UpdateCCDBin(int binx, int biny) override;

    // Guide Port
    virtual IPState GuideNorth(float) override;
    virtual IPState GuideSouth(float) override;
    virtual IPState GuideEast(float) override;
    virtual IPState GuideWest(float) override;

    // Filter Wheel CFW
    virtual int QueryFilter() override;
    virtual bool SelectFilter(int position) override;

#ifndef __APPLE__
    // Streaming
    virtual bool StartStreaming() override;
    virtual bool StopStreaming() override;
#endif

    ISwitch CoolerS[2];
    ISwitchVectorProperty CoolerSP;

    INumber CoolerN[1];
    INumberVectorProperty CoolerNP;

    INumber GainN[1];
    INumberVectorProperty GainNP;

    INumber OffsetN[1];
    INumberVectorProperty OffsetNP;

    INumber SpeedN[1];
    INumberVectorProperty SpeedNP;

    INumber USBTrafficN[1];
    INumberVectorProperty USBTrafficNP;

  private:
    // Get time until next image is due
    float calcTimeLeft();
    // Get image buffer from camera
    int grabImage();
    // Setup basic CCD parameters on connection
    bool setupParams();
    // Enable/disable cooler
    void setCooler(bool enable);
    // Check if the camera is QHY5PII-C model
    bool isQHY5PIIC();

    // Temperature update
    void updateTemperature();
    static void updateTemperatureHelper(void *);

    char name[MAXINDIDEVICE];
    char camid[MAXINDIDEVICE];

    // CCD dimensions
    int camxbin;
    int camybin;
    int camroix;
    int camroiy;
    int camroiwidth;
    int camroiheight;

    // CCD extra capabilities
    bool HasUSBTraffic;
    bool HasUSBSpeed;
    bool HasGain;
    bool HasOffset;
    bool HasFilters;

    qhyccd_handle *camhandle;
    INDI::CCDChip::CCD_FRAME imageFrameType;
    bool sim;

    // Temperature tracking
    float TemperatureRequest;
    int temperatureID;
    bool coolerEnabled;//, useSoftBin;

    // Exposure progress
    float ExposureRequest;
    float LastExposureRequest;
    struct timeval ExpStart;
    int timerID;

    // Gain
    float GainRequest;
    float LastGainRequest;

    // Threading
    int streamPredicate;
    pthread_t primary_thread;
    bool terminateThread;
    pthread_cond_t cv         = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;

    friend void ::ISGetProperties(const char *dev);
    friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
    friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
    friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
    friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                            char *formats[], char *names[], int n);
    friend void ::ISSnoopDevice(XMLEle *root);
};
