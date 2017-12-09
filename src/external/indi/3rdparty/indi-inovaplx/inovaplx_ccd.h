/*
   INDI Driver for i-Nova PLX series
   Copyright 2013/2014 i-Nova Technologies - Ilia Platone

   Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)
*/

#pragma once

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <memory>
#include <pthread.h>
#include <indiccd.h>

#include <inovasdk.h>

int instanceN = 0;
class INovaCCD : public INDI::CCD
{
public:
    INovaCCD();

    bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    void ISGetProperties(const char *dev);

    void CaptureThread();
protected:

    // General device functions
    bool Connect();
    bool Disconnect();
    const char *getDeviceName();
    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();

    // CCD specific functions
    bool StartExposure(float duration);
    bool AbortExposure();
    void TimerHit();
    void addFITSKeywords(fitsfile *fptr, INDI::CCDChip *targetChip);

    // Guiding
    IPState GuideEast(float ms);
    IPState GuideWest(float ms);
    IPState GuideNorth(float ms);
    IPState GuideSouth(float ms);

private:

    // Utility functions
    float CalcTimeLeft();
    void  setupParams();
    void  grabImage();

    // Are we exposing?
    bool InExposure;
    bool threadsRunning;
//    bool FrameReady;

    unsigned char *RawData;
//    unsigned char *TmpData;

    // Struct to keep timing
    struct timeval ExpStart;
    float ExposureRequest;
//    int GainRequest;
//    int BlackLevelRequest;
//    int timerID;
    pthread_t captureThread;       

    // We declare the CCD properties
    IText iNovaInformationT[5];
    ITextVectorProperty iNovaInformationTP;

    INumber CameraPropertiesN[2];
    INumberVectorProperty CameraPropertiesNP;
    enum
    {
        CCD_GAIN_N,
        CCD_BLACKLEVEL_N,
    };


};

