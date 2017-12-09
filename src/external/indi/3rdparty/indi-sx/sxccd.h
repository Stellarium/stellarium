/*
  Starlight Xpress CCD INDI Driver

  Copyright (c) 2012-2013 Cloudmakers, s. r. o.
  All Rights Reserved.

  Code is based on SX INDI Driver by Gerry Rozema and Jasem Mutlaq
  Copyright(c) 2010 Gerry Rozema.
  Copyright(c) 2012 Jasem Mutlaq.
  All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*/

#pragma once

#include "sxccdusb.h"

#include <indiccd.h>

void ExposureTimerCallback(void *p);
void GuideExposureTimerCallback(void *p);
void WEGuiderTimerCallback(void *p);
void NSGuiderTimerCallback(void *p);

class SXCCD : public INDI::CCD
{
  private:
    DEVICE device;
    HANDLE handle;
    unsigned short model;
    char name[32];
    char *evenBuf, *oddBuf;
    long wipeDelay;
    ISwitch CoolerS[2];
    ISwitchVectorProperty CoolerSP;
    ISwitch ShutterS[2];
    ISwitchVectorProperty ShutterSP;
    float TemperatureRequest;
    float TemperatureReported;
    float ExposureTimeLeft;
    float GuideExposureTimeLeft;
    int ExposureTimerID;
    int GuideExposureTimerID;
    int WEGuiderTimerID;
    int NSGuiderTimerID;
    bool DidFlush;
    bool DidLatch;
    bool DidGuideLatch;
    bool InGuideExposure;
    char GuideStatus;

  protected:
    const char *getDefaultName();
    bool initProperties();
    void getCameraParams();
    bool updateProperties();
    bool UpdateCCDFrame(int x, int y, int w, int h);
    bool UpdateCCDBin(int hor, int ver);
    bool Connect();
    bool Disconnect();
    int SetTemperature(double temperature);
    bool StartExposure(float n);
    bool AbortExposure();
    bool StartGuideExposure(float n);
    bool AbortGuideExposure();
    void TimerHit();
    void ExposureTimerHit();
    void GuideExposureTimerHit();
    void WEGuiderTimerHit();
    void NSGuiderTimerHit();
    IPState GuideWest(float time);
    IPState GuideEast(float time);
    IPState GuideNorth(float time);
    IPState GuideSouth(float time);

  public:
    bool HasCooler;
    bool HasShutter;
    bool HasST4Port;
    bool HasGuideHead;
    SXCCD(DEVICE device, const char *name);
    virtual ~SXCCD();
    void debugTriggered(bool enable);
    void simulationTriggered(bool enable);
    void ISGetProperties(const char *dev);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

    friend void ::ExposureTimerCallback(void *p);
    friend void ::GuideExposureTimerCallback(void *p);
    friend void ::WEGuiderTimerCallback(void *p);
    friend void ::NSGuiderTimerCallback(void *p);
    friend void ::ISGetProperties(const char *dev);
    friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
    friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
    friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
    friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                            char *formats[], char *names[], int n);
    friend void ::ISSnoopDevice(XMLEle *root);
};
