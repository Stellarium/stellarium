/*
    FLI CCD
    INDI Interface for Finger Lakes Instrument CCDs
    Copyright (C) 2003-2016 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

    2016.05.16: Added CCD Cooler Power (JM)

*/

#pragma once

#include <libfli.h>
#include <indiccd.h>
#include <iostream>

using namespace std;

class FLICCD : public INDI::CCD
{
  public:
    FLICCD();
    virtual ~FLICCD();

    const char *getDefaultName();

    bool initProperties();
    void ISGetProperties(const char *dev);
    bool updateProperties();

    bool Connect();
    bool Disconnect();

    bool StartExposure(float duration);
    bool AbortExposure();

    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    void TimerHit();
    virtual int SetTemperature(double temperature);
    virtual bool UpdateCCDFrame(int x, int y, int w, int h);
    virtual bool UpdateCCDBin(int binx, int biny);
    virtual bool UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType);

    virtual void debugTriggered(bool enable);

  private:
    // Find FLI CCD
    bool findFLICCD(flidomain_t domain);

    // Calculate Time until exposure is complete
    float calcTimeLeft();

    // Fetch image row by row from the CCD
    bool grabImage();

    // Get initial CCD values upon connection
    bool setupParams();

    typedef struct
    {
        flidomain_t domain;
        char *dname;
        char *name;
        char model[32];
        long HWRevision;
        long FWRevision;
        double x_pixel_size;
        double y_pixel_size;
        long Array_Area[4];
        long Visible_Area[4];
        int width, height;
        double temperature;
    } cam_t;

    ISwitch PortS[4];
    ISwitchVectorProperty PortSP;

    IText CamInfoT[3];
    ITextVectorProperty CamInfoTP;

    INumber CoolerN[1];
    INumberVectorProperty CoolerNP;

    double minDuration;
    int timerID;

    // Exposure timing
    struct timeval ExpStart;
    float ExposureRequest;

    flidev_t fli_dev;
    cam_t FLICam;

    // Simulation mode
    bool sim;
};
