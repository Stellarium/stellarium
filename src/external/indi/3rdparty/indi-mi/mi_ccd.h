/*
 Moravian INDI Driver

 Copyright (C) 2015 Jasem Mutlaq (mutlaqja@ikarustech.com)
 Copyright (C) 2016 Jakub Smutny (linux@gxccd.com)

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

#include "gxccd.h"

#include <indiccd.h>
#include <indifilterinterface.h>

class MICCD : public INDI::CCD, public INDI::FilterInterface
{
  public:
    MICCD(int cameraId, bool eth = false);
    virtual ~MICCD();

    virtual const char *getDefaultName() override;

    virtual bool initProperties() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool updateProperties() override;

    virtual bool Connect() override;
    virtual bool Disconnect() override;

    virtual int SetTemperature(double temperature) override;
    virtual bool StartExposure(float duration) override;
    virtual bool AbortExposure() override;

    virtual IPState GuideNorth(float) override;
    virtual IPState GuideSouth(float) override;
    virtual IPState GuideEast(float) override;
    virtual IPState GuideWest(float) override;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    // Misc.
    virtual void TimerHit() override;
    virtual bool saveConfigItems(FILE *fp) override;

    // CCD
    virtual bool UpdateCCDFrame(int x, int y, int w, int h) override;
    virtual bool UpdateCCDBin(int binx, int biny) override;

    // Filter Wheel CFW
    virtual int QueryFilter() override;
    virtual bool SelectFilter(int position) override;    

    INumber FanN[1];
    INumberVectorProperty FanNP;

    INumber WindowHeatingN[1];
    INumberVectorProperty WindowHeatingNP;

    INumber CoolerN[1];
    INumberVectorProperty CoolerNP;

    INumber TemperatureRampN[1];
    INumberVectorProperty TemperatureRampNP;

    INumber GainN[1];
    INumberVectorProperty GainNP;

    ISwitch ReadModeS[3];
    ISwitchVectorProperty ReadModeSP;

  private:
    char name[MAXINDIDEVICE];

    int cameraId;
    camera_t *cameraHandle;
    bool isEth;

    bool hasGain;
    bool useShutter;

    int numReadModes;
    int numFilters;
    float minExpTime;
    int maxFanValue;
    int maxHeatingValue;
    int maxBinX;
    int maxBinY;

    int temperatureID;
    int timerID;

    bool downloading;

    INDI::CCDChip::CCD_FRAME imageFrameType;

    float TemperatureRequest;
    float ExposureRequest;
    struct timeval ExpStart;

    bool setupParams();

    float calcTimeLeft();
    int grabImage();

    void updateTemperature();
    static void updateTemperatureHelper(void *);

    friend void ::ISGetProperties(const char *dev);
    friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
    friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
    friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
    friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                            char *formats[], char *names[], int n);
    friend void ::ISSnoopDevice(XMLEle *root);
};
