/*******************************************************************************
  Copyright(c) 2016 Andy Kirkham. All rights reserved.

 HitecAstroDCFocuser Focuser

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

#include "hidapi.h"
#include "indifocuser.h"
#include "indiusbdevice.h"

class HitecAstroDCFocuser : public INDI::Focuser, public INDI::USBDevice
{
  public:
    typedef enum { IDLE, SLEWING } STATE;

    HitecAstroDCFocuser();
    virtual ~HitecAstroDCFocuser();

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool saveConfigItems(FILE *fp);

    bool Connect();
    bool Disconnect();

    void TimerHit();

    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);

    virtual IPState MoveFocuser(FocusDirection dir, int speed, uint16_t duration);

  private:
    hid_device *_handle;

    bool sim;

    char _stop;
    STATE _state;
    uint16_t _duration;

    INumber MaxPositionN[1];
    INumberVectorProperty MaxPositionNP;

    INumber SlewSpeedN[1];
    INumberVectorProperty SlewSpeedNP;

    ISwitch ReverseDirectionS[1];
    ISwitchVectorProperty ReverseDirectionSP;
};
