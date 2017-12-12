/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  INDI MBox Driver

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#pragma once

#include "indiweather.h"

class MBox : public INDI::Weather
{
  public:
    MBox();
    virtual ~MBox() = default;

    //  Generic indi device entries
    virtual bool Handshake() override;
    virtual const char *getDefaultName() override;

    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    virtual IPState updateWeather() override;    

  private:
    typedef enum { ACK_OK_STARTUP, ACK_OK_INIT, ACK_ERROR } AckResponse;
    typedef enum { CAL_PRESSURE, CAL_TEMPERATURE, CAL_HUMIDITY } CalibrationType;

    AckResponse ack();

    bool verifyCRC(const char *response);
    bool getCalibration(bool sendCommand=true);
    bool setCalibration(CalibrationType type);
    bool resetCalibration();

    INumber CalibrationN[3];
    INumberVectorProperty CalibrationNP;


    ISwitch ResetS[1];
    ISwitchVectorProperty ResetSP;

    IText FirmwareT[1];
    ITextVectorProperty FirmwareTP;



};
