/**
 * INDI driver for Meade DSI.
 *
 * Copyright (C) 2015 Ben Gilsrud
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

#pragma once

#include <indiccd.h>

namespace DSI
{
class Device;
}

class DSICCD : public INDI::CCD
{
  public:
    DSICCD();

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    // General device functions
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;

    // CCD specific functions
    virtual bool UpdateCCDBin(int hor, int ver) override;
    virtual bool StartExposure(float duration) override;
    virtual bool AbortExposure() override;
    virtual void TimerHit() override;

    // misc functions
    virtual bool saveConfigItems(FILE *fp) override;

  private:
    // Utility functions
    float CalcTimeLeft();
    void setupParams();
    void grabImage();

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

    INumber GainN[1];
    INumberVectorProperty GainNP;

    INumber CCDTempN[1];
    INumberVectorProperty CCDTempNP;

    ISwitch VddExpS[2];
    ISwitchVectorProperty VddExpSP;

    INumber OffsetN[1];
    INumberVectorProperty OffsetNP;

    DSI::Device *dsi;
};
