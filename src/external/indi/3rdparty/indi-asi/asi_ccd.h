/*
 ASI CCD Driver

 Copyright (C) 2015 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "ASICamera2.h"

#include <indiccd.h>

class ASICCD : public INDI::CCD
{
  public:
    explicit ASICCD(ASI_CAMERA_INFO *camInfo);
    virtual ~ASICCD();

    virtual const char *getDefaultName() override;

    virtual bool initProperties() override;
    virtual bool updateProperties() override;

    virtual bool Connect() override;
    virtual bool Disconnect() override;

    virtual int SetTemperature(double temperature) override;
    virtual bool StartExposure(float duration) override;
    virtual bool AbortExposure() override;

    static void *streamVideoHelper(void *context);
    void *streamVideo();

  protected:
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

// Streaming
    virtual bool StartStreaming() override;
    virtual bool StopStreaming() override;

    virtual void TimerHit() override;
    virtual bool UpdateCCDFrame(int x, int y, int w, int h) override;
    virtual bool UpdateCCDBin(int binx, int biny) override;

    // Guide Port
    virtual IPState GuideNorth(float ms) override;
    virtual IPState GuideSouth(float ms) override;
    virtual IPState GuideEast(float ms) override;
    virtual IPState GuideWest(float ms) override;

    // ASI specific keywords
    virtual void addFITSKeywords(fitsfile *fptr, INDI::CCDChip *targetChip) override;

    // Save config
    virtual bool saveConfigItems(FILE *fp) override;

  private:
    /** Get image from CCD and send it to client */
    int grabImage();
    /** Get initial parameters from camera */
    bool setupParams();
    /** Calculate time left in seconds after start_time */
    float calcTimeLeft(float duration, timeval *start_time);
    /** Create number and switch controls for camera by querying the API */
    void createControls(int piNumberOfControls);
    /** Get the current Bayer string used */
    const char *getBayerString();
    /** Update control values from camera */
    void updateControls();
    /** Return user selected image type */
    ASI_IMG_TYPE getImageType();
    /** Update SER recorder video format */
    void updateRecorderFormat();
    /** Control cooler */
    bool activateCooler(bool enable);
    /** Set Video Format */
    bool setVideoFormat(uint8_t index);

    char name[MAXINDIDEVICE];

    /** Additional Properties to INDI::CCD */
    INumber CoolerN[1];
    INumberVectorProperty CoolerNP;

    ISwitch CoolerS[2];
    ISwitchVectorProperty CoolerSP;

    INumber *ControlN = nullptr;
    INumberVectorProperty ControlNP;

    ISwitch *ControlS = nullptr;
    ISwitchVectorProperty ControlSP;

    ISwitch *VideoFormatS;
    ISwitchVectorProperty VideoFormatSP;
    uint8_t rememberVideoFormat = { 0 };
    ASI_IMG_TYPE currentVideoFormat;

    double minimumExposureDuration = 0;
    struct timeval ExpStart;
    float ExposureRequest;
    float TemperatureRequest;
    int TemperatureUpdateCounter;

    ASI_CAMERA_INFO *m_camInfo;
    ASI_CONTROL_CAPS *pControlCaps;

    bool sim;

    // Threads
    int streamPredicate;
    bool terminateThread;
    pthread_t primary_thread;
    pthread_cond_t cv         = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;

    int exposureRetries;

    // ST4
    bool InWEPulse;
    float WEPulseRequest;
    struct timeval WEPulseStart;
    int WEtimerID;

    bool InNSPulse;
    float NSPulseRequest;
    struct timeval NSPulseStart;
    int NStimerID;

    ASI_GUIDE_DIRECTION WEDir;
    ASI_GUIDE_DIRECTION NSDir;

    friend void ::ISGetProperties(const char *dev);
    friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
    friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
    friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
    friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                            char *formats[], char *names[], int n);
};
