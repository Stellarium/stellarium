/*******************************************************************************
  Copyright(c) 2015 Camiel Severijns. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "indifocuser.h"

class SmartFocus : public INDI::Focuser
{
  public:
    SmartFocus();

    const char *getDefaultName() override;

    bool initProperties() override;
    bool updateProperties() override;

    virtual bool Handshake() override;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    //virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n) override;

    virtual bool AbortFocuser() override;
    virtual IPState MoveAbsFocuser(uint32_t targetPosition) override;
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
    virtual void TimerHit() override;

    typedef unsigned short Position;
  protected:
    virtual bool saveConfigItems(FILE *fp) override;

  private:
    typedef unsigned char Flags;

    enum State
    {
        Idle,
        MovingTo,
        MovingIn,
        MovingOut
    };

    enum StatusFlags
    {
        STATUS_SERIAL_FRAMING_ERROR = 0,
        STATUS_SERIAL_OVERRUN_ERROR,
        STATUS_MOTOR_ENCODE_ERROR,
        STATUS_AT_ZERO_POSITION,
        STATUS_AT_MAX_POSITION,
        STATUS_NUM_FLAGS
    };

    enum FlagBits
    {
        SerFramingError   = 0x02,
        SerOverrunError   = 0x04,
        MotorEncoderError = 0x08,
        AtZeroPosition    = 0x40,
        AtMaxPosition     = 0x80
    };

    bool SFacknowledge();
    bool SFisIdle() const { return (state == Idle); }
    bool SFisMoving() const { return !SFisIdle(); }
    Position SFgetPosition();
    Flags SFgetFlags();
    void SFgetState();

    bool send(const char *command, const size_t nbytes, const char *from, const bool log_error = true);
    bool recv(char *respons, const size_t nbytes, const char *from, const bool log_error = true);

    Position position { 0 };
    State state { Idle };
    int timer_id { 0 };

    ILight FlagsL[5];
    ILightVectorProperty FlagsLP;
    INumber MaxPositionN[1];
    INumberVectorProperty MaxPositionNP;
};
