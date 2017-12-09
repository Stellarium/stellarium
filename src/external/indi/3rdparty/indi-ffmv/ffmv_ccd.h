/**
 * INDI driver for Point Grey FireFly MV camera.
 *
 * Copyright (C) 2013 Ben Gilsrud
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef FFMVCCD_H
#define FFMVCCD_H

#include <indiccd.h>
#include <dc1394/dc1394.h>

using namespace std;

class FFMVCCD : public INDI::CCD
{
  public:
    FFMVCCD();

    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    // General device functions
    bool Connect();
    bool Disconnect();
    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();

    // CCD specific functions
    bool StartExposure(float duration);
    bool AbortExposure();
    void TimerHit();

  private:
    // Utility functions
    float CalcTimeLeft();
    void setupParams();
    void grabImage();
    dc1394error_t writeMicronReg(unsigned int offset, unsigned int val);
    dc1394error_t readMicronReg(unsigned int offset, unsigned int *val);

    dc1394error_t setGainVref(ISState iss);
    dc1394error_t setDigitalGain(ISState state);

    // Are we exposing?
    bool InExposure;
    bool capturing;
    // Struct to keep timing
    struct timeval ExpStart;

    float ExposureRequest;
    float TemperatureRequest;
    int timerID;
    float max_exposure;
    float last_exposure_length;
    int sub_count;

    ISwitch GainS[2];
    ISwitchVectorProperty GainSP;
    // We declare the CCD temperature property
    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    dc1394_t *dc1394;
    dc1394camera_t *dcam;

    float last_duration;
};

#endif // FFMVCCD_H
