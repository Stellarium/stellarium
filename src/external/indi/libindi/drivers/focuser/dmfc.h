/*
    Pegasus DMFC Focuser
    Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indifocuser.h"

class DMFC : public INDI::Focuser
{
  public:
    DMFC();
    virtual ~DMFC() = default;

    virtual bool Handshake();
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

protected:
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual bool AbortFocuser();
    virtual void TimerHit();
    virtual bool saveConfigItems(FILE *fp);

  private:
    bool updateFocusParams();
    bool sync(uint32_t newPosition);
    bool move(uint32_t newPosition);
    bool setMaxSpeed(uint16_t speed);
    bool setReverseEnabled(bool enable);
    bool setLedEnabled(bool enable);
    bool setEncodersEnabled(bool enable);
    bool setBacklash(uint16_t value);
    bool setMotorType(uint8_t type);
    bool ack();

    uint32_t currentPosition { 0 };
    uint32_t targetPosition { 0 };
    bool isMoving = false;

    // Temperature probe
    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    // Sync Position
    INumber SyncN[1];
    INumberVectorProperty SyncNP;

    // Motor Mode
    ISwitch MotorTypeS[2];
    ISwitchVectorProperty MotorTypeSP;
    enum { MOTOR_DC, MOTOR_STEPPER };

    // Rotator Encoders
    ISwitch EncoderS[2];
    ISwitchVectorProperty EncoderSP;
    enum { ENCODERS_ON, ENCODERS_OFF };

    // Enable/Disable backlash
    ISwitch BacklashCompensationS[2];
    ISwitchVectorProperty BacklashCompensationSP;
    enum { BACKLASH_ENABLED, BACKLASH_DISABLED };

    // Backlash Value
    INumber BacklashN[1];
    INumberVectorProperty BacklashNP;

    // Reverse Direction
    ISwitch ReverseS[2];
    ISwitchVectorProperty ReverseSP;
    enum { DIRECTION_NORMAL, DIRECTION_REVERSED };

    // LED
    ISwitch LEDS[2];
    ISwitchVectorProperty LEDSP;
    enum { LED_OFF, LED_ON };

    // Maximum Speed
    INumber MaxSpeedN[1];
    INumberVectorProperty MaxSpeedNP;

    // Firmware Version
    IText FirmwareVersionT[1];
    ITextVectorProperty FirmwareVersionTP;
};
